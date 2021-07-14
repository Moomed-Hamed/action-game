#include "player.h"

#define TARGET_FRAMES_PER_SECOND ((float)120)
#define DRAW_DISTANCE 512.0f
#define FOV ToRadians(45.0f)

int main()
{
	Window   window = {};
	Mouse    mouse  = {};
	Keyboard keys   = {};

	init_window(&window, 1920, 1080, "action game");
	init_keyboard(&keys);

	Camera camera = { vec3(0,1,0) };

	Asset_Renderer* axe_renderer = Alloc(Asset_Renderer, 1);
	init(axe_renderer);

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

		mat4 proj_view = proj * glm::lookAt(camera.position, camera.position + camera.front, camera.up);

		// renderer updates
		update_renderer(axe_renderer, camera.position, camera.up, camera.front);
		update_renderer(terrain_renderer);

		// Geometry pass
		glBindFramebuffer(GL_FRAMEBUFFER, g_buffer.FBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		bind(axe_renderer->shader);
		set_mat4(axe_renderer->shader, "proj_view", proj_view);
		bind_texture(axe_renderer->mesh, 4);
		draw(axe_renderer->mesh, 1);

		draw(terrain_renderer, proj_view);

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