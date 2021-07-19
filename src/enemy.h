#include "bullets.h"

// (FABRIK) inverse kinematics + ai = active ragdolls (probably need an animation update too)
struct Enemy
{
	Sphere_Collider feet; // for movement, shouldn't be neccessary in multiplayer game
	Cylinder_Collider hitbox;

	float health = 100;
};

void init(Enemy* enemy)
{
	//init_collider(&enemy->feet  , vec3(3, 5, 3), vec3(0, 0, 0), .5);
	//init_collider(&enemy->hitbox, vec3(0, 1, 0), vec3(0, 0, 0), 1, 1);
}
void update(Enemy* enemy, float dtime)
{
//	update_collider(&enemy->feet, dtime);
	enemy->hitbox.position = enemy->feet.position;
}

// rendering

struct Enemy_Drawable
{
	vec3 position;
	mat3 rotation;
};

struct Enemy_Renderer
{
	Drawable_Mesh_Anim mesh;
	Shader shader;
	Animation animation;
	mat4 current_pose[MAX_ANIMATED_BONES];
};

void init(Enemy_Renderer* renderer)
{
	load(&renderer->mesh, "assets/meshes/enemy.mesh_anim", sizeof(Enemy_Drawable));
	mesh_add_attrib_vec3(4, sizeof(Enemy_Drawable), 0); // world pos
	mesh_add_attrib_mat3(5, sizeof(Enemy_Drawable), sizeof(vec3)); // rotation

	load(&(renderer->shader), "assets/shaders/mesh_anim.vert", "assets/shaders/mesh.frag");
	bind(renderer->shader);
	set_int(renderer->shader, "positions", 0);
	set_int(renderer->shader, "normals"  , 1);
	set_int(renderer->shader, "albedo"   , 2);

	load(&renderer->animation, "assets/animations/enemy_run.anim"); // animaiton keyframes
	GLint skeleton_id = glGetUniformBlockIndex(renderer->shader.id, "skeleton");
	glUniformBlockBinding(renderer->shader.id, skeleton_id, 0);
}
void update_renderer(Enemy_Renderer* renderer, Enemy enemy, float dtime)
{
	update_animation(&renderer->animation, renderer->current_pose, dtime);

	//static vec3 position = vec3(0, 0, 0);
	//position += dtime * vec3(-2.5, 0, 0);

	Enemy_Drawable drawable = {};
	drawable.position = enemy.feet.position + vec3(-5, 1, -5);
	drawable.rotation = mat3(.2) * point_at(vec3(0, 0, 1), vec3(0, 1, 0));

	update(renderer->mesh, renderer->animation.num_bones, renderer->current_pose, sizeof(Enemy_Drawable), (byte*)(&drawable));
}
void draw(Enemy_Renderer* renderer, mat4 proj_view)
{
	bind(renderer->shader);
	set_mat4(renderer->shader, "proj_view", proj_view);
	draw(renderer->mesh);
}