#include "camera.h"

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

	Camera camera = {};

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

		mat4 proj_view = proj * glm::lookAt(camera.position, camera.position + camera.front, camera.up);

		// Geometry pass
		glBindFramebuffer(GL_FRAMEBUFFER, g_buffer.FBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// draw meshes

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