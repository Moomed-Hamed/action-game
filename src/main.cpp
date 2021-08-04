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
	//init_window(&window, 2560, 1440, "action game");
	init_keyboard(&keys);

	Bullet* bullets = Alloc(Bullet, MAX_BULLETS);
	Bullet_Renderer* bullet_renderer = Alloc(Bullet_Renderer, 1);
	init(bullet_renderer);

	Player* player = Alloc(Player, 1);
	init(player);

	Peer* peer = Alloc(Peer, 1);
	init(peer);

	Peer_Renderer* peer_renderer = Alloc(Peer_Renderer, 1);
	init(peer_renderer);

	Enemy* enemies = Alloc(Enemy, MAX_ENEMIES);
	init(enemies);
	Enemy_Renderer* enemy_renderer = Alloc(Enemy_Renderer, 1);
	init(enemy_renderer);

	Crosshair_Renderer* gui = Alloc(Crosshair_Renderer, 1);
	init(gui);

	Particle_Emitter* emitter = Alloc(Particle_Emitter, 1);
	Particle_Renderer* particle_renderer = Alloc(Particle_Renderer, 1);
	init(particle_renderer);

	Gun gun = {};
	gun.shoot = load_audio("assets/audio/pistol_shot.audio");
	Gun_Renderer* gun_renderer = Alloc(Gun_Renderer, 1);
	init(gun_renderer);

	Orb* orbs = Alloc(Orb, MAX_ORBS);
	Orb_Renderer* orb_renderer = Alloc(Orb_Renderer, 1);
	init(orb_renderer);

	Pickup* pickups = Alloc(Pickup, MAX_PICKUPS);
	Pickup_Renderer* pickup_renderer = Alloc(Pickup_Renderer, 1);
	init(pickup_renderer);
	pickups[0].position = vec3(1.5, .6, 1.5);

	Physics_Colliders* colliders = Alloc(Physics_Colliders, 1);
	init_colldier(colliders->dynamic.cubes    , vec3(1, .5, 3), vec3(0, 0, 0), vec3(0, 0, 0), 1, vec3(1, 1, 1));
	init_collider(colliders->dynamic.cylinders, vec3(5, .5, 3), vec3(0, 0, 0), vec3(0, 0, 0), 1, 1, .5);
	init_collider(colliders->dynamic.spheres  , vec3(3, .5, 3), vec3(0, 0, 0), vec3(0, 0, 0), 1, .5);
	//init_collider(colliders->fixed.planes, vec3(5, 6, 6), vec3(0, 0, 0), vec3(0, 0, 0), vec3(0, 0, -1), vec2(10, 10));
	//init_collider(colliders->fixed.planes + 1, vec3(0, 0, 0), vec3(0, 0, 0), vec3(0, 0, 0), vec3(0, 1, 0), vec2(100, 100));

	Physics_Renderer* physics_renderer = Alloc(Physics_Renderer, 1);
	init(physics_renderer);

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
	//create_thread(server_proc, peer);
	//create_thread(client_proc, &level->player);

	Audio orb = load_audio("assets/audio/orb.audio");
	Audio headshot = load_audio("assets/audio/headshot.audio");

	while (1)
	{
		update_window(window);
		update_mouse(&mouse, window);
		update_keyboard(&keys, window);

		if (keys.ESC.is_pressed) break;

		if (keys.U.is_pressed) {
			static float metal = 0.0;
			metal += frame_time;
			if (metal > 1) metal = 0;
			set_float(lighting_shader, "metallic", metal);
		}
		if (keys.I.is_pressed) {
			static float metal = 0.0;
			metal += frame_time;
			if (metal > 1) metal = 0;
			set_float(lighting_shader, "roughness", metal);
		}
		if (keys.O.is_pressed) {
			static float metal = 0.0;
			metal += frame_time;
			if (metal > 1) metal = 0;
			set_float(lighting_shader, "ao", metal);
		}

		if (keys.G.is_pressed) player->eyes.trauma = 1;
		if (keys.G.is_pressed && !keys.G.was_pressed) play_audio(headshot);

		static float a = 0; a += frame_time;
		if (a > .2) { a = 0; spawn_fire(emitter); }

		// game updates
		update(player , keys, mouse, frame_time);
		update(orbs   , player->eyes.position, frame_time, orb);
		update(&gun   , bullets, &player->eyes, mouse, keys, frame_time);
		update(emitter, frame_time, vec3(0));
		update(bullets, enemies, frame_time);
		update(enemies, orbs, emitter, &player->eyes, frame_time);

		// renderer updates
		update(gui);
		update_renderer(particle_renderer, emitter);
		update_renderer(physics_renderer , colliders);
		update_renderer(peer_renderer    , *peer, frame_time);
		update_renderer(orb_renderer     , orbs);
		update_renderer(pickup_renderer  , pickups, frame_time);
		update_renderer(gun_renderer     , gun, frame_time, player->eyes, mouse.norm_dx);
		update_renderer(bullet_renderer  , bullets);
		update_renderer(enemy_renderer   , enemies);
		update_renderer(tile_renderer);

		// Geometry pass
		glBindFramebuffer(GL_FRAMEBUFFER, g_buffer.FBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		mat4 proj_view = proj * lookAt(player->eyes.position, player->eyes.position + player->eyes.front, player->eyes.up);
		draw(gui);
		draw(particle_renderer , proj_view);
		draw(tile_renderer     , proj_view);
		draw(physics_renderer  , proj_view);
		draw(peer_renderer     , proj_view);
		//draw(orb_renderer      , proj_view);
		draw(pickup_renderer   , proj_view);
		draw(gun_renderer      , proj_view);
		draw(bullet_renderer   , proj_view);
		draw(enemy_renderer    , proj_view);

		// Lighting pass
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		bind(lighting_shader);
		set_vec3(lighting_shader, "view_pos", player->eyes.position);
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