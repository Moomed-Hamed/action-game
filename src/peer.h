#include "props.h"

// (FABRIK) inverse kinematics + ai = active ragdolls (probably need an animation update too)
struct Peer
{
	vec3 feet_position;
	vec3 look_direction;
	Cylinder_Collider hitbox;

	float health = 100;
};

void init(Peer* peer)
{
	init_collider(&peer->hitbox, vec3(0, 1, 0), vec3(0, 0, 0), vec3(0, 0, 0), 1, 1, .5);
}
void update(Peer* peer, float dtime)
{
	peer->hitbox.position = peer->feet_position;
}

// rendering

struct Peer_Drawable
{
	vec3 position;
	mat3 rotation;
};

struct Peer_Renderer
{
	Drawable_Mesh_Anim_UV mesh;
	Shader shader;
	Animation animation;
	mat4 current_pose[MAX_ANIMATED_BONES];
};

void init(Peer_Renderer* renderer)
{
	load(&renderer->mesh, "assets/meshes/peer.mesh_anim", "assets/textures/palette.bmp", sizeof(Peer_Drawable));
	mesh_add_attrib_vec3(5, sizeof(Peer_Drawable), 0); // world pos
	mesh_add_attrib_mat3(6, sizeof(Peer_Drawable), sizeof(vec3)); // rotation

	load(&(renderer->shader), "assets/shaders/mesh_anim_uv.vert", "assets/shaders/mesh_uv.frag");
	bind(renderer->shader);
	set_int(renderer->shader, "positions", 0);
	set_int(renderer->shader, "normals"  , 1);
	set_int(renderer->shader, "albedo"   , 2);
	set_int(renderer->shader, "texture_sampler", 3);

	load(&renderer->animation, "assets/animations/peer.anim"); // animaiton keyframes
	GLint skeleton_id = glGetUniformBlockIndex(renderer->shader.id, "skeleton");
	glUniformBlockBinding(renderer->shader.id, skeleton_id, 1); // this is a hack & needs more attention
	glBindBufferBase(GL_UNIFORM_BUFFER, 1, renderer->mesh.UBO);
}
void update_renderer(Peer_Renderer* renderer, Peer peer, float dtime)
{
	update_animation(&renderer->animation, renderer->current_pose, dtime * 3);

	Peer_Drawable drawable = {};
	drawable.position = peer.feet_position;
	drawable.rotation = mat3(.2);// *point_at(-1.f * vec3(peer.look_direction.x, 0, peer.look_direction.z), vec3(0, 1, 0));

	update(renderer->mesh, renderer->animation.num_bones, renderer->current_pose, sizeof(Peer_Drawable), (byte*)(&drawable));
}
void draw(Peer_Renderer* renderer, mat4 proj_view)
{
	bind(renderer->shader);
	set_mat4(renderer->shader, "proj_view", proj_view);
	bind_texture(renderer->mesh);
	glBindBuffer(GL_UNIFORM_BUFFER, 1);
	draw(renderer->mesh);
}