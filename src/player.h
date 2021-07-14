#include "terrain.h"

// rendering

struct Player_Arms_Drawable
{
	vec3 position;
	mat3 rotation;
};

struct Asset_Renderer
{
	Drawable_Mesh_UV mesh;
	Shader shader;
};

void init(Asset_Renderer* renderer)
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
void update_renderer(Asset_Renderer* renderer, vec3 pos, vec3 up, vec3 front)
{
	Player_Arms_Drawable arms = {};

	arms.position = pos;
	arms.rotation = point_at(-1.f * front, up);

	update(renderer->mesh, sizeof(Player_Arms_Drawable), (byte*)(&arms));
}