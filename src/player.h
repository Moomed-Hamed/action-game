#include "barrel.h"

#define FOV ToRadians(45.0f)

#define DIR_FORWARD	0
#define DIR_BACKWARD	1
#define DIR_LEFT	2
#define DIR_RIGHT	3

struct Camera
{
	vec3 position;
	vec3 front, right, up;
	float yaw, pitch;
};

void camera_update_dir(Camera* camera, float dx, float dy, float sensitivity = 0.003)
{
	camera->yaw   += (dx * sensitivity) / TWOPI;
	camera->pitch += (dy * sensitivity) / TWOPI;

	if (camera->pitch >  PI / 2.01) camera->pitch =  PI / 2.01;
	if (camera->pitch < -PI / 2.01) camera->pitch = -PI / 2.01;

	camera->front.y = sin(camera->pitch);
	camera->front.x = cos(camera->pitch) * cos(camera->yaw);
	camera->front.z = cos(camera->pitch) * sin(camera->yaw);

	camera->front = normalize(camera->front);
	camera->right = normalize(cross(camera->front, vec3(0, 1, 0)));
	camera->up    = normalize(cross(camera->right, camera->front));
}
void camera_update_pos(Camera* camera, int direction, float distance)
{
	if (direction == DIR_FORWARD ) camera->position += camera->front * distance;
	if (direction == DIR_LEFT    ) camera->position -= camera->right * distance;
	if (direction == DIR_RIGHT   ) camera->position += camera->right * distance;
	if (direction == DIR_BACKWARD) camera->position -= camera->front * distance;
}

// rendering

struct Player_Arms_Drawable
{
	vec3 position;
	mat3 rotation;
};

struct Player_Arms_Renderer
{
	Drawable_Mesh_UV mesh;
	Shader shader;
};

void init(Player_Arms_Renderer* renderer)
{
	load(&renderer->mesh, "assets/meshes/rifle.mesh_uv", "assets/textures/palette.bmp", sizeof(Player_Arms_Drawable));
	mesh_add_attrib_vec3(3, sizeof(Player_Arms_Drawable), 0); // world pos
	mesh_add_attrib_mat3(4, sizeof(Player_Arms_Drawable), sizeof(vec3)); // rotation

	load(&(renderer->shader), "assets/shaders/mesh_uv_rot.vert", "assets/shaders/mesh_uv.frag");
	bind(renderer->shader);
	set_int(renderer->shader, "positions", 0);
	set_int(renderer->shader, "normals", 1);
	set_int(renderer->shader, "albedo", 2);
	set_int(renderer->shader, "texture_sampler", 4);
}
void update_renderer(Player_Arms_Renderer* renderer, Camera cam, float dtime, float turn)
{
	Player_Arms_Drawable arms = {};

	static float time = 0; time += dtime;

	arms.position = cam.position;
	arms.position += cam.up * (.01f * sin(time * 2.6f));
	//arms.position += cam.right * (.005f * sin(time * 2.6f));
	arms.rotation = point_at(cam.front, cam.up);

	update(renderer->mesh, sizeof(Player_Arms_Drawable), (byte*)(&arms));
}
void draw(Player_Arms_Renderer* renderer, mat4 proj_view)
{
	bind(renderer->shader);
	set_mat4(renderer->shader, "proj_view", proj_view);
	bind_texture(renderer->mesh, 4);
	draw(renderer->mesh, 1);
}