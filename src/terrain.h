#include "particles.h"

struct Terrain_Drawable
{
	vec3 position;
	mat3 rotation;
};

struct Terrain_Renderer
{
	Drawable_Mesh_UV mesh;
	Shader shader;
};

void init(Terrain_Renderer* renderer)
{
	load(&renderer->mesh, "assets/meshes/ground.mesh_uv", "assets/textures/palette.bmp", sizeof(Terrain_Drawable));
	mesh_add_attrib_vec3(3, sizeof(Terrain_Drawable), 0); // world pos
	mesh_add_attrib_mat3(4, sizeof(Terrain_Drawable), sizeof(vec3)); // rotation

	load(&(renderer->shader), "assets/shaders/mesh_uv_rot.vert", "assets/shaders/mesh_uv.frag");
	bind(renderer->shader);
	set_int(renderer->shader, "positions", 0);
	set_int(renderer->shader, "normals"  , 1);
	set_int(renderer->shader, "albedo"   , 2);
	set_int(renderer->shader, "texture_sampler", 3);
}
void update_renderer(Terrain_Renderer* renderer)
{
	Terrain_Drawable terrain = {};

	terrain.position = vec3(0, 0, 0);
	terrain.rotation = mat3(1.f);

	update(renderer->mesh, sizeof(Terrain_Drawable), (byte*)(&terrain));
}
void draw(Terrain_Renderer* renderer, mat4 proj_view)
{
	bind(renderer->shader);
	set_mat4(renderer->shader, "proj_view", proj_view);
	bind_texture(renderer->mesh, 3);
	draw(renderer->mesh, 1);
}

// floors

struct Level_Floor
{
	Plane_Collider_Static collider;
};

