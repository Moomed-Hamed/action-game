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
	
	Ocean ocean = {};
	init(&ocean, normalize(vec2(1, 0)) * 10.f);
	calculate_waves(ocean, 0);

	Props* props = Alloc(Props, 1); init(props);
	Prop_Renderer* prop_renderer = Alloc(Prop_Renderer, 1);
	init(prop_renderer);

	Heightmap* heightmap = Alloc(Heightmap, 1);
	Heightmap_Renderer* heightmap_renderer = Alloc(Heightmap_Renderer, 1);
	init(heightmap_renderer, heightmap, "assets/textures/heightmap.r32");

	Sea_Renderer* sea_renderer = Alloc(Sea_Renderer, 1);
	init(sea_renderer, ocean.height, ocean.normal);

	Bullet* bullets = Alloc(Bullet, MAX_BULLETS);
	Bullet_Renderer* bullet_renderer = Alloc(Bullet_Renderer, 1);
	init(bullet_renderer);

	Player* player = Alloc(Player, 1); init(player);
	Player_Renderer* player_renderer = Alloc(Player_Renderer, 1);
	init(player_renderer);

	Peer* peer = Alloc(Peer, 1); init(peer);
	Peer_Renderer* peer_renderer = Alloc(Peer_Renderer, 1);
	init(peer_renderer);

	Enemy* enemies = Alloc(Enemy, MAX_ENEMIES); init(enemies);
	Enemy_Renderer* enemy_renderer = Alloc(Enemy_Renderer, 1);
	init(enemy_renderer);

	Crosshair_Renderer* gui = Alloc(Crosshair_Renderer, 1);
	init(gui);

	Particle_Emitter* emitter = Alloc(Particle_Emitter, 1);
	Particle_Renderer* particle_renderer = Alloc(Particle_Renderer, 1);
	init(particle_renderer);

	Orb* orbs = Alloc(Orb, MAX_ORBS);
	Orb_Renderer* orb_renderer = Alloc(Orb_Renderer, 1);
	init(orb_renderer);

	Physics_Colliders* colliders = Alloc(Physics_Colliders, 1);
	colliders->dynamic.cubes     [0] = { {1, .5, 3}, {}, {}, 1, vec3(1) };
	colliders->dynamic.cylinders [0] = { {5, .5, 3}, {}, {}, 1, 1, .5 };
	colliders->dynamic.spheres   [0] = { {3, .5, 3}, {}, {}, 1, .5 };
	//colliders->fixed.planes[0] = { {5, 6, 6}, {}, {}, {0, 0, -1}, vec2(10)  };
	//colliders->fixed.planes[1] = { {0, 0, 0}, {}, {}, {0, 1,  0}, vec2(100) };

	Physics_Renderer* physics_renderer = Alloc(Physics_Renderer, 1);
	init(physics_renderer);

	G_Buffer g_buffer = make_g_buffer(window);
	Shader lighting_shader = make_lighting_shader();
	mat4 proj = glm::perspective(FOV, (float)window.screen_width / window.screen_height, 0.1f, DRAW_DISTANCE);

	// frame timer
	float frame_time = 1.f / 45;
	int64 target_frame_milliseconds = frame_time * 1000.f; // seconds * 1000 = milliseconds
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

		if (keys.G.is_pressed) player->eyes.trauma = 1;
		if (keys.G.is_pressed && !keys.G.was_pressed) play_audio(headshot);

		static float a = 0; a += frame_time;
		if (a > .2) { a = 0; emit_fire(emitter); }

		// game updates
		update(player , frame_time, bullets, keys, mouse); peer->look_direction = player->eyes.front;
		update(enemies, frame_time, orbs, emitter, &player->eyes);
		update(orbs   , frame_time, player->eyes.position, orb);
		update(bullets, frame_time, enemies);
		update(emitter, frame_time, vec3(0));

		// renderer updates
		update(gui);
		update_renderer(sea_renderer      , frame_time, player->eyes.position);
		update_renderer(player_renderer   , frame_time, *player, mouse);
		update_renderer(enemy_renderer    , frame_time, enemies);
		update_renderer(peer_renderer     , frame_time, *peer);
		update_renderer(particle_renderer , emitter);
		update_renderer(bullet_renderer   , bullets);
		update_renderer(orb_renderer      , orbs);
		//update_renderer(tile_renderer);
		update_renderer(prop_renderer, props);

		static float time = 0; time += frame_time / 4;
		calculate_waves(ocean, time);

		static float i = 0;
		if (keys.J.is_pressed && !keys.J.was_pressed)
			recalculate(&ocean, normalize(vec2(1, 0)) * 5.f, .001 + i++ * .01);

		// Geometry pass
		glBindFramebuffer(GL_FRAMEBUFFER, g_buffer.FBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		mat4 proj_view = proj * lookAt(player->eyes.position, player->eyes.position + player->eyes.front, player->eyes.up);
		
		draw(sea_renderer      , proj_view);
		draw(player_renderer   , proj_view);
		draw(enemy_renderer    , proj_view);
		draw(particle_renderer , proj_view);
		draw(orb_renderer      , proj_view);
		draw(bullet_renderer   , proj_view);
		draw(prop_renderer     , proj_view);
		draw(heightmap_renderer, proj_view);
		draw(physics_renderer, proj_view);
		//draw(peer_renderer   , proj_view);
		draw(gui);

		// Lighting pass
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		bind(lighting_shader);
		set_vec3(lighting_shader, "view_pos", player->eyes.position);
		draw(g_buffer);

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