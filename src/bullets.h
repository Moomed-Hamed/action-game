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
void update(Bullet* bullets, float dtime, Enemy* enemies)
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
				if (point_in_cylinder(bullets[i].position, enemies[j].collider))
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
	vec3 scale, color;
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
	load(&renderer->mesh, "assets/meshes/basic/ico.mesh", sizeof(renderer->bullets));
	mesh_add_attrib_vec3(2, sizeof(Bullet_Drawable), 0 * sizeof(vec3)); // position
	mesh_add_attrib_vec3(3, sizeof(Bullet_Drawable), 1 * sizeof(vec3)); // scale
	mesh_add_attrib_vec3(4, sizeof(Bullet_Drawable), 2 * sizeof(vec3)); // color
	mesh_add_attrib_mat3(5, sizeof(Bullet_Drawable), 3 * sizeof(vec3)); // rotation

	load(&(renderer->shader), "assets/shaders/transform/mesh.vert", "assets/shaders/mesh.frag");
}
void update_renderer(Bullet_Renderer* renderer, Bullet* bullets)
{
	for (uint i = 0; i < MAX_BULLETS; i++)
	{
		if (bullets[i].type > 0)
		{
			renderer->bullets[i].position  = bullets[i].position;
			renderer->bullets[i].transform = mat3(1);
			renderer->bullets[i].scale     = vec3(.01);
			renderer->bullets[i].color     = vec3(.3);
		} else renderer->bullets[i] = {};
	}

	update(renderer->mesh, MAX_BULLETS * sizeof(Bullet_Drawable), (byte*)(&renderer->bullets));
}
void draw(Bullet_Renderer* renderer, mat4 proj_view)
{
	bind(renderer->shader);
	set_mat4(renderer->shader, "proj_view", proj_view);
	draw(renderer->mesh, MAX_BULLETS);
}