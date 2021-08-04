#include "horde.h"

#define MAX_BULLETS 16

struct Bullet
{
	uint type;
	vec3 position, velocity;
	float damage;
	float time_alive;
};

void spawn(Bullet* bullets, vec3 position, vec3 velocity, uint type = 1)
{
	for (uint i = 0; i < MAX_BULLETS; i++)
	{
		if (bullets[i].type == NULL)
		{
			bullets[i] = {};
			bullets[i].type = type;
			bullets[i].position = position;
			bullets[i].velocity = velocity;
			bullets[i].time_alive = 0;
			return;
		}
	}
}
void update(Bullet* bullets, Enemy* enemies, float dtime)
{
	for (uint i = 0; i < MAX_BULLETS; i++)
	{
		if (bullets[i].type > 0)
		{
			bullets[i].position += bullets[i].velocity * dtime;
			bullets[i].time_alive += dtime;
			if(bullets[i].time_alive > 5) bullets[i] = {};

			for (uint j = 0; j < MAX_ENEMIES; j++)
			{
				if (point_in_sphere(bullets[i].position, enemies[j].collider))
				{
					enemies[j].health -= 10;
					enemies[j].trauma += .25;
					bullets[i] = {};
					break;
				}
			}
		}
	}
}

// rendering

struct Bullet_Drawable
{
	vec3 position;
	vec3 scale;
	vec3 color;
	mat3 transform;
};

struct Bullet_Renderer
{
	Bullet_Drawable bullets[MAX_BULLETS];
	Drawable_Mesh mesh;
	Shader shader;
};

void init(Bullet_Renderer* renderer)
{
	load(&renderer->mesh, "assets/meshes/basic/sphere.mesh", MAX_ENEMIES * sizeof(Enemy_Drawable));
	mesh_add_attrib_vec3(2, sizeof(Enemy_Drawable), 0 * sizeof(vec3)); // position
	mesh_add_attrib_vec3(3, sizeof(Enemy_Drawable), 1 * sizeof(vec3)); // scale
	mesh_add_attrib_vec3(4, sizeof(Enemy_Drawable), 2 * sizeof(vec3)); // color
	mesh_add_attrib_mat3(5, sizeof(Enemy_Drawable), 3 * sizeof(vec3)); // rotation

	load(&(renderer->shader), "assets/shaders/particle.vert", "assets/shaders/mesh.frag");
	bind(renderer->shader);
	set_int(renderer->shader, "positions", 0);
	set_int(renderer->shader, "normals"  , 1);
	set_int(renderer->shader, "albedo"   , 2);
}
void update_renderer(Bullet_Renderer* renderer, Bullet* bullets)
{
	for (uint i = 0; i < MAX_BULLETS; i++)
	{
		if (bullets[i].type > 0)
		{
			renderer->bullets[i].position = bullets[i].position;
			renderer->bullets[i].transform = mat3(1);
			renderer->bullets[i].scale = vec3(.01);
			renderer->bullets[i].color = vec3(0,1,1);
		}
		else
		{
			renderer->bullets[i] = {};
		}
	}

	update(renderer->mesh, MAX_BULLETS * sizeof(Bullet_Drawable), (byte*)(&renderer->bullets));
}
void draw(Bullet_Renderer* renderer, mat4 proj_view)
{
	bind(renderer->shader);
	set_mat4(renderer->shader, "proj_view", proj_view);
	draw(renderer->mesh, MAX_BULLETS);
}

// pickups (health, ammo, etc.)
#define MAX_PICKUPS 1

struct Pickup
{
	vec3 position;
};

// rendering

struct Pickup_Drawable
{
	vec3 position;
	mat3 transform;
	//vec2 texture_offset;
};

struct Pickup_Renderer
{
	uint num_pickups;
	Pickup_Drawable pickups[MAX_PICKUPS];
	Drawable_Mesh_UV mesh;
	Shader shader;
};

void init(Pickup_Renderer* renderer)
{
	load(&renderer->mesh, "assets/meshes/ammo.mesh_uv", "assets/textures/palette.bmp", MAX_PICKUPS * sizeof(Pickup_Drawable));
	mesh_add_attrib_vec3(3, sizeof(Pickup_Drawable), 0); // world pos
	mesh_add_attrib_mat3(4, sizeof(Pickup_Drawable), sizeof(vec3)); // transform

	load(&(renderer->shader), "assets/shaders/mesh_uv_rot.vert", "assets/shaders/mesh_uv.frag");
	bind(renderer->shader);
	set_int(renderer->shader, "positions", 0);
	set_int(renderer->shader, "normals"  , 1);
	set_int(renderer->shader, "albedo"   , 2);
	set_int(renderer->shader, "texture_sampler", 3);
}
void update_renderer(Pickup_Renderer* renderer, Pickup* pickups, float dtime)
{
	static float spin_timer = 0; spin_timer += dtime;
	if (spin_timer > TWOPI) spin_timer = 0;

	renderer->num_pickups = 0;

	for (uint i = 0; i < MAX_PICKUPS; i++)
	{
		renderer->num_pickups += 1;
		renderer->pickups[i].position = pickups[i].position;
		renderer->pickups[i].transform = glm::rotate(spin_timer, vec3(0, 1, 0));
	}

	update(renderer->mesh, renderer->num_pickups * sizeof(Pickup_Drawable), (byte*)(renderer->pickups));
}
void draw(Pickup_Renderer* renderer, mat4 proj_view)
{
	bind(renderer->shader);
	set_mat4(renderer->shader, "proj_view", proj_view);
	bind_texture(renderer->mesh, 3);
	draw(renderer->mesh, renderer->num_pickups);
}