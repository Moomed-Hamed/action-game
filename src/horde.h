#include "peer.h"

#define MAX_ENEMIES 1

struct Enemy
{
	uint type;
	Cylinder_Collider collider;
	float health, trauma;
};

void init(Enemy* enemies)
{
	for (uint i = 0; i < MAX_ENEMIES; i++)
	{
		enemies[i].type = 1;
		enemies[i].health = 5;
		enemies[i].trauma = 0;

		vec3 position = vec3(random_chance_signed(), 0, random_chance_signed()) * 5.f;
		position.y = .5;

		init_collider(&enemies[i].collider, position, vec3(0), vec3(0), 1, 1, .5);
	}
}
void update(Enemy* enemies, float dtime, Orb* orbs, Particle_Emitter* emitter, Camera* cam)
{
	for (uint i = 0; i < MAX_ENEMIES; i++)
	{
		if (enemies[i].type > 0)
		{
			if (enemies[i].health < 0)
			{
				for (uint j = 0; j < 8; j++) spawn(orbs, enemies[i].collider.position);
				emit_explosion(emitter, enemies[i].collider.position + vec3(0, .25, 0));
				cam->trauma += .5;
				enemies[i] = {};
			}
			else
			{
				enemies[i].collider.position += enemies[i].collider.velocity * dtime;
				enemies[i].trauma -= dtime;
				if (enemies[i].trauma < 0) enemies[i].trauma = 0;
			}
		}
	}
}

// rendering

struct Enemy_Renderer
{
	Prop_Drawable enemies[MAX_ENEMIES];
	Drawable_Mesh_Anim_UV mesh;
	Shader shader;
	Animation animation;
	mat4 current_pose[MAX_ANIMATED_BONES];
};

void init(Enemy_Renderer* renderer)
{
	load(&renderer->mesh, "assets/meshes/skeleton.mesh_anim", sizeof(renderer->enemies));
	mesh_add_attrib_vec3(5, sizeof(Prop_Drawable), 0); // world pos
	mesh_add_attrib_mat3(6, sizeof(Prop_Drawable), sizeof(vec3)); // rotation

	renderer->mesh.texture_id  = load_texture("assets/textures/palette2.bmp");
	renderer->mesh.material_id = load_texture("assets/textures/materials.bmp");

	load(&(renderer->shader), "assets/shaders/transform/mesh_anim_uv.vert", "assets/shaders/mesh_uv.frag");
	load(&renderer->animation, "assets/animations/skeleton.anim"); // animaiton keyframes
}
void update_renderer(Enemy_Renderer* renderer, float dtime, Enemy* enemies)
{
	for (uint i = 0; i < MAX_ENEMIES; i++)
	{
		if (enemies[i].type > 0)
		{
			vec3 offset = shake(enemies[i].trauma) * .1f;

			renderer->enemies[i].position  = enemies[i].collider.position + offset - vec3(0, .5, 0);
			renderer->enemies[i].transform = mat3(1 + offset.x);
		} else { renderer->enemies[i] = {}; }
	}

	update_animation(&renderer->animation, renderer->current_pose, dtime);

	update(renderer->mesh, renderer->animation.num_bones, renderer->current_pose, sizeof(renderer->enemies), (byte*)(&renderer->enemies));
}
void draw(Enemy_Renderer* renderer, mat4 proj_view)
{
	bind(renderer->shader);
	set_mat4(renderer->shader, "proj_view", proj_view);
	bind_texture(renderer->mesh);
	draw(renderer->mesh, MAX_ENEMIES);
}