#include "player.h"

#define TARGET_FRAMES_PER_SECOND ((float)120)
#define DRAW_DISTANCE 512.0f

int main()
{
	Window   window = {};
	Mouse    mouse  = {};
	Keyboard keys   = {};

	init_window(&window, 1920, 1080, "action game");
	init_keyboard(&keys);

	Camera camera = {};

	GUI_Renderer* gui = Alloc(GUI_Renderer, 1);
	init(gui);

	gui->num_quads = 1;
	gui->quads[0].position = vec2(0, 0);
	gui->quads[0].scale    = vec2(.003, .005);
	gui->quads[0].color    = vec3(1, 0, 0);

	Physics_Colliders* colliders = Alloc(Physics_Colliders, 1);
	colliders->cubes[0]      = { vec3(1, 5, 3), vec3(0, 0, 0), vec3(1, 1, 1) };
	colliders->spheres[0]    = { vec3(3, 5, 3), vec3(0, 0, 0), .5            };
	colliders->cylinders[0]  = { vec3(5, 5, 3), vec3(0, 0, 0), .5, 1         };
	colliders->planes[0]     = { vec3(5,0.5,6), vec2(1, 1), vec3(0, 0,-1) };
	colliders->planes[1]     = { vec3(0, 0, 0), vec2(100, 1), vec3(0, 1, 0) };

	Physics_Renderer* physics_renderer = Alloc(Physics_Renderer, 1);
	init(physics_renderer);

	Player_Arms_Renderer* player_arms_renderer = Alloc(Player_Arms_Renderer, 1);
	init(player_arms_renderer);

	Bullet* bullets = Alloc(Bullet, MAX_BULLETS);
	Bullet_Renderer* bullet_renderer = Alloc(Bullet_Renderer, 1);
	init(bullet_renderer);

	Terrain_Renderer* terrain_renderer = Alloc(Terrain_Renderer, 1);
	init(terrain_renderer);

	G_Buffer g_buffer = {};
	init_g_buffer(&g_buffer, window);
	Shader lighting_shader = make_lighting_shader();
	mat4 proj = glm::perspective(FOV, (float)window.screen_width / window.screen_height, 0.1f, DRAW_DISTANCE);

	// frame timer
	float frame_time = 1.f / 60;
	int64 target_frame_milliseconds = frame_time * 1000.f;
	Timestamp frame_start = get_timestamp(), frame_end;

	while (1)
	{
		update_window(window);
		update_mouse(&mouse, window);
		update_keyboard(&keys, window);

		if (keys.ESC.is_pressed) break;

		if (keys.W.is_pressed) camera_update_pos(&camera, DIR_FORWARD , 5 * frame_time);
		if (keys.S.is_pressed) camera_update_pos(&camera, DIR_BACKWARD, 5 * frame_time);
		if (keys.A.is_pressed) camera_update_pos(&camera, DIR_LEFT    , 5 * frame_time);
		if (keys.D.is_pressed) camera_update_pos(&camera, DIR_RIGHT   , 5 * frame_time);
		camera.position.y = 1.8;
		camera_update_dir(&camera, mouse.dx, mouse.dy);

		if (mouse.left_button.is_pressed && !mouse.left_button.was_pressed)
		{
			bullets[0].position = camera.position + (camera.front * .36f);
			bullets[0].velocity = camera.front * 10.f;
			bullets[0].up = camera.up;
		}

		mat4 proj_view = proj * glm::lookAt(camera.position, camera.position + camera.front, camera.up);

		//physics updates
		if (keys.R.is_pressed) colliders->spheres[0].velocity += camera.front;
		if (keys.F.is_pressed) colliders->spheres[0].velocity -= camera.front;
		update_colliders(colliders, frame_time);

		// game updates
		update_bullets(bullets, colliders, frame_time);

		// renderer updates
		update_renderer(physics_renderer, colliders);
		update_renderer(player_arms_renderer, camera, frame_time, mouse.norm_x);
		update_renderer(terrain_renderer);
		update_renderer(bullet_renderer, bullets);

		// Geometry pass
		glBindFramebuffer(GL_FRAMEBUFFER, g_buffer.FBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		update(gui);
		bind(gui->shader);
		draw(gui);

		draw(player_arms_renderer, proj_view);
		draw(terrain_renderer, proj_view);
		draw(physics_renderer, proj_view);
		draw(bullet_renderer , proj_view);

		// Lighting pass
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		bind(lighting_shader);
		set_vec3(lighting_shader, "view_pos", camera.position);
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