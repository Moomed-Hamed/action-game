#include "enemy.h"

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
	float trauma;
};

void camera_update_dir(Camera* camera, float dx, float dy, float dtime, float sensitivity = 0.003)
{
	// camera shake
	float trauma = camera->trauma;

	static uint offset = random_uint() % 16;

	if (camera->trauma > 1) camera->trauma = 1;
	if (camera->trauma > 0) camera->trauma -= dtime;
	else
	{
		camera->trauma = 0;
		offset = random_uint() % 16;
	}

	float p1 = ((perlin((trauma + offset + 0) * 1000) * 2) - 1) * trauma;
	float p2 = ((perlin((trauma + offset + 1) * 2000) * 2) - 1) * trauma;
	float p3 = ((perlin((trauma + offset + 2) * 3000) * 2) - 1) * trauma;

	float shake_yaw   = ToRadians(p1);
	float shake_pitch = ToRadians(p2);
	float shake_roll  = ToRadians(p3);

	camera->yaw   += (dx * sensitivity) / TWOPI;
	camera->pitch += (dy * sensitivity) / TWOPI;

	float yaw   = camera->yaw + shake_yaw;
	float pitch = camera->pitch + shake_pitch;

	// it feels a little different (better?) if we let the shake actually move the camera a little
	//camera->yaw   += shake_yaw;
	//camera->pitch += shake_pitch;

	// updating camera direction
	if (camera->pitch >  PI / 2.01) camera->pitch =  PI / 2.01;
	if (camera->pitch < -PI / 2.01) camera->pitch = -PI / 2.01;

	camera->front.y = sin(pitch);
	camera->front.x = cos(pitch) * cos(yaw);
	camera->front.z = cos(pitch) * sin(yaw);

	camera->front = normalize(camera->front);
	camera->right = normalize(cross(camera->front, vec3(0, 1, 0)));
	camera->up    = normalize(cross(camera->right, camera->front));

	mat3 roll = glm::rotate(shake_roll, camera->front);
	camera->up = roll * camera->up;
}
void camera_update_pos(Camera* camera, int direction, float distance)
{
	if (direction == DIR_FORWARD ) camera->position += camera->front * distance;
	if (direction == DIR_LEFT    ) camera->position -= camera->right * distance;
	if (direction == DIR_RIGHT   ) camera->position += camera->right * distance;
	if (direction == DIR_BACKWARD) camera->position -= camera->front * distance;
}

struct Player
{
	Camera eyes;
	Sphere_Collider feet;
};

void init(Player* player)
{
	init_collider(&player->feet, vec3(0, 10, 0), vec3(0, 0, 0), vec3(0, 0, 0), 1, .25);
}
void update(Player* player, Keyboard keys, Mouse mouse, float dtime, float shake)
{
	// i don't really have any idea why this works but id does so it stays 4 now

	Sphere_Collider* feet = &player->feet;

	camera_update_dir(&(player->eyes), mouse.dx, mouse.dy, shake);

	// decoupling movement controls from physics
	float side_velocity    = 0;
	float forward_velocity = 0;

	if (keys.W.is_pressed) forward_velocity =  6.f;
	if (keys.S.is_pressed) forward_velocity = -6.f;
	if (keys.A.is_pressed) side_velocity    = -6.f;
	if (keys.D.is_pressed) side_velocity    =  6.f;

	vec3 movement_velocity = (player->eyes.front * forward_velocity) + (player->eyes.right * side_velocity);
	movement_velocity.y = 0;

	feet->position += dtime * movement_velocity;

	Plane_Collider ground = {};
	init_collider(&ground, vec3(0, 0, 0), vec3(0, 0, 0), vec3(0, 0, 0), vec3(0, 1, 0), vec2(100, 100));

	vec3 force = vec3(feet->force.x, feet->force.y + GRAVITY, feet->force.z);

	float bounce_impulse = 0;
	if (sphere_plane_intersect(*feet, ground) && dot(feet->velocity, ground.normal) < 0)
	{
		if (dot(feet->velocity, ground.normal) < 0) bounce_impulse = feet->velocity.y * -1.f;
	
		float penetration_depth = abs(feet->position.y - feet->radius); // distance from contact point to plane
		feet->velocity = penetration_depth * ground.normal * 10.f;
	}

	feet->velocity += dtime * ((force / feet->mass) + bounce_impulse);
	feet->position += dtime * feet->velocity;

	static float jump_timer  = -1;
	static float jump_offset = 0;

	if (jump_timer < 0 && keys.SPACE.is_pressed && !keys.SPACE.was_pressed)
	{
		jump_timer = 1 / 1.5f;
	}

	if (jump_timer > 0)
	{
		jump_offset = 1.5 * sin(jump_timer * 1.5 * PI);
		jump_timer -= dtime;
	}

	player->eyes.position   = player->feet.position;
	player->eyes.position.y = player->feet.position.y + 1.3f + jump_offset;

	//// weapons
	//if (mouse.left_button.is_pressed && player->eyes.trauma < .25)
	//{
	//	player->eyes.trauma += .1;
	//}
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
	set_int(renderer->shader, "normals"  , 1);
	set_int(renderer->shader, "albedo"   , 2);
	set_int(renderer->shader, "texture_sampler", 4);
}
void update_renderer(Player_Arms_Renderer* renderer, Camera cam, float dtime, bool firing, float turn)
{
	Player_Arms_Drawable arms = {};

	static float time = 0; time += dtime;

	arms.position = cam.position;

	if (firing)
	{
		arms.position -= cam.front * (.01f * sin(time * TWOPI * 10));
		arms.position += cam.right * (.005f * sin(time * 2.6f));
	}
	else
	{
		arms.position += cam.up    * (.01f * sin(time * 2.6f));
		arms.position += cam.right * (.005f * sin(time * 1.6f));
	}
	
	//static vec3 turnvec = {};
	//turnvec = cam.right * (turn / 10);

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

// crosshair rendering

struct Quad_Drawable
{
	vec2 position;
	vec2 scale;
	vec3 color;
};

struct Crosshair_Renderer
{
	Quad_Drawable quads[4];
	Drawable_Quad mesh;
	Shader shader;
};

void init(Crosshair_Renderer* renderer)
{
	*renderer = {};
	init(&renderer->mesh, 4 * sizeof(Quad_Drawable));
	mesh_add_attrib_vec2(1, sizeof(Quad_Drawable), 0); // position
	mesh_add_attrib_vec2(2, sizeof(Quad_Drawable), sizeof(vec2)); // scale
	mesh_add_attrib_vec3(3, sizeof(Quad_Drawable), sizeof(vec2) + sizeof(vec2)); // color

	load(&renderer->shader, "assets/shaders/gui.vert", "assets/shaders/gui.frag");

	vec2 position = vec2(0, 0);
	vec2 scale    = vec2(.003, .005) / 2.f;
	vec3 color    = vec3(1, 0, 0);

	renderer->quads[0].position = position + vec2(.01f, 0);
	renderer->quads[0].scale    = scale;
	renderer->quads[0].color    = color;
	renderer->quads[1].position = position - vec2(.01f, 0);
	renderer->quads[1].scale    = scale;
	renderer->quads[1].color    = color;
	renderer->quads[2].position = position + vec2(0, .01f);
	renderer->quads[2].scale    = scale;
	renderer->quads[2].color    = color;
	renderer->quads[3].position = position - vec2(0, .01f);
	renderer->quads[3].scale    = scale;
	renderer->quads[3].color    = color;

	update(renderer->mesh, 4 * sizeof(Quad_Drawable), (byte*)renderer->quads);
}
void draw(Crosshair_Renderer* renderer)
{
	bind(renderer->shader);
	draw(renderer->mesh, 4);
}