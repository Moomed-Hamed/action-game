#include "networking.h"

#define TARGET_FRAMES_PER_SECOND ((float)120)
#define DRAW_DISTANCE 512.0f

int main()
{
	Window   window = {};
	Mouse    mouse  = {};
	Keyboard keys   = {};

	//init_window(&window, 1280, 720, "action game");
	init_window(&window, 1920, 1080, "action game");
	init_keyboard(&keys);

	Level* level = Alloc(Level, 1);
	init(level);

	Enemy_Renderer* enemy_renderer = Alloc(Enemy_Renderer, 1);
	init(enemy_renderer);

	Crosshair_Renderer* gui = Alloc(Crosshair_Renderer, 1);
	init(gui);

	Particle_Emitter* emitter = Alloc(Particle_Emitter, 1);
	init(emitter);

	Particle_Renderer* particle_renderer = Alloc(Particle_Renderer, 1);
	init(particle_renderer);

	Drawable_Mesh_2D_UV test = {};
	init(&test, "assets/textures/palette.bmp");

	Physics_Colliders* colliders = Alloc(Physics_Colliders, 1);
	init_colldier(colliders->dynamic.cubes    , vec3(1, .5, 3), vec3(0, 0, 0), vec3(0, 0, 0), 1, vec3(1, 1, 1));
	init_collider(colliders->dynamic.cylinders, vec3(5, .5, 3), vec3(0, 0, 0), vec3(0, 0, 0), 1, 1, .5);
	init_collider(colliders->dynamic.spheres  , vec3(3, .5, 3), vec3(0, 0, 0), vec3(0, 0, 0), 1, .5);
	//init_collider(colliders->fixed.planes, vec3(5, 6, 6), vec3(0, 0, 0), vec3(0, 0, 0), vec3(0, 0, -1), vec2(10, 10));
	//init_collider(colliders->fixed.planes + 1, vec3(0, 0, 0), vec3(0, 0, 0), vec3(0, 0, 0), vec3(0, 1, 0), vec2(100, 100));

	Physics_Renderer* physics_renderer = Alloc(Physics_Renderer, 1);
	init(physics_renderer);

	Player_Arms_Renderer* player_arms_renderer = Alloc(Player_Arms_Renderer, 1);
	init(player_arms_renderer);

	Bullet_Renderer* bullet_renderer = Alloc(Bullet_Renderer, 1);
	init(bullet_renderer);

	Tile_Renderer* tile_renderer = Alloc(Tile_Renderer, 1);
	init(tile_renderer);

	G_Buffer g_buffer = {};
	init_g_buffer(&g_buffer, window);
	Shader lighting_shader = make_lighting_shader();
	mat4 proj = glm::perspective(FOV, (float)window.screen_width / window.screen_height, 0.1f, DRAW_DISTANCE);

	// frame timer
	float frame_time = 1.f / 60;
	int64 target_frame_milliseconds = frame_time * 1000.f;
	Timestamp frame_start = get_timestamp(), frame_end;

	// networking
	create_thread(server_proc, &level->enemy); srand(__rdtsc());
	//create_thread(client_proc, &level->player);

	while (1)
	{
		update_window(window);
		update_mouse(&mouse, window);
		update_keyboard(&keys, window);

		if (keys.ESC.is_pressed) break;

		update(&level->player, keys, mouse, frame_time, frame_time);

		if (mouse.left_button.is_pressed && !mouse.left_button.was_pressed)
		{
			level->bullets[0].position = level->player.eyes.position + (level->player.eyes.front * .36f);
			level->bullets[0].velocity = level->player.eyes.front * 10.f;
			level->bullets[0].up = level->player.eyes.up;
		}

		mat4 proj_view = proj * glm::lookAt(level->player.eyes.position, level->player.eyes.position + level->player.eyes.front, level->player.eyes.up);

		//physics updates
		if (keys.R.is_pressed) colliders->dynamic.spheres[0].velocity += level->player.eyes.front;
		if (keys.F.is_pressed) colliders->dynamic.spheres[0].velocity -= level->player.eyes.front;
		if (keys.G.is_pressed) level->player.eyes.trauma = 1;
		//update_colliders(colliders, frame_time);

		// game updates
		update_level(level, frame_time);
		update(emitter, frame_time / 3);
		level->enemy.look_direction = level->player.eyes.right * -1.f;

		// renderer updates
		update(gui);
		update_renderer(particle_renderer, emitter, level->player.eyes.front);
		update_renderer(physics_renderer, colliders);
		update_renderer(player_arms_renderer, level->player.eyes, frame_time, mouse.left_button.is_pressed, mouse.norm_x);
		update_renderer(tile_renderer);
		update_renderer(bullet_renderer, (Bullet*)&(level->bullets));
		update_renderer(enemy_renderer, level->enemy, frame_time);

		// Geometry pass
		glBindFramebuffer(GL_FRAMEBUFFER, g_buffer.FBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		draw(gui);
		draw(particle_renderer   , proj_view);
		draw(player_arms_renderer, proj_view);
		draw(tile_renderer       , proj_view);
		draw(physics_renderer    , proj_view);
		draw(bullet_renderer     , proj_view);
		draw(enemy_renderer      , proj_view);

		// Lighting pass
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		bind(lighting_shader);
		set_vec3(lighting_shader, "view_pos", level->player.eyes.position);
		draw_g_buffer(g_buffer);

		//Frame Time
		frame_end = get_timestamp();
		int64 milliseconds_elapsed = calculate_milliseconds_elapsed(frame_start, frame_end);

		//print("frame time: %02d ms | fps: %06f\n", milliseconds_elapsed, 1000.f / milliseconds_elapsed);
		if (target_frame_milliseconds > milliseconds_elapsed) // frame finished early
		{
			os_sleep(target_frame_milliseconds - milliseconds_elapsed);
		}
		
		frame_start = frame_end;
	}

	shutdown_window();
	return 0;
}