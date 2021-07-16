#include "terrain.h"

#define MAX_BULLETS 16

struct Bullet
{
	vec3 position;
	vec3 velocity;
	vec3 up;
	float damage;
};

void update_bullets(Bullet* bullets, Physics_Colliders* colliders, float dtime)
{
	for (uint i = 0; i < MAX_BULLETS; i++)
	{
		if (point_in_sphere(bullets[i].position, colliders->spheres[0]))
		{
			colliders->spheres[0].velocity += bullets[0].velocity;
			bullets[i] = {};
		}
		else
		{
			bullets[i].position += bullets[i].velocity * dtime;
		}
	}
}

// rendering

struct Bullet_Drawable
{
	vec3 position;
	mat3 rotation;
};

struct Bullet_Renderer
{
	Drawable_Mesh_UV mesh;
	Shader shader;
};

void init(Bullet_Renderer* renderer)
{
	load(&renderer->mesh, "assets/meshes/bullet.mesh_uv", "assets/textures/palette.bmp", sizeof(Bullet_Drawable));
	mesh_add_attrib_vec3(3, sizeof(Bullet_Drawable), 0); // world pos
	mesh_add_attrib_mat3(4, sizeof(Bullet_Drawable), sizeof(vec3)); // rotation

	load(&(renderer->shader), "assets/shaders/mesh_uv_rot.vert", "assets/shaders/mesh_uv.frag");
	bind(renderer->shader);
	set_int(renderer->shader, "positions", 0);
	set_int(renderer->shader, "normals"  , 1);
	set_int(renderer->shader, "albedo"   , 2);
	set_int(renderer->shader, "texture_sampler", 3);
}
void update_renderer(Bullet_Renderer* renderer, Bullet* bullets)
{
	Bullet_Drawable bullet = {};

	bullet.position = bullets[0].position;
	bullet.rotation = point_at(normalize(bullets[0].velocity), bullets[0].up);

	update(renderer->mesh, sizeof(Bullet_Drawable), (byte*)(&bullet));
}
void draw(Bullet_Renderer* renderer, mat4 proj_view)
{
	bind(renderer->shader);
	set_mat4(renderer->shader, "proj_view", proj_view);
	bind_texture(renderer->mesh, 3);
	draw(renderer->mesh, 1);
}