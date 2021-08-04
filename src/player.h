#include "guns.h"

#define FOV ToRadians(45.0f)

struct Player
{
	Camera eyes;
	Sphere_Collider feet;
};

void init(Player* player)
{
	init_collider(&player->feet, vec3(0, 1, 0), vec3(0, 0, 0), vec3(0, 0, 0), 1, .25);
}
void update(Player* player, Keyboard keys, Mouse mouse, float dtime)
{
	// i don't really have any idea why this works but id does so it stays 4 now

	Sphere_Collider* feet = &player->feet;

	camera_update_dir(&(player->eyes), mouse.dx, mouse.dy, dtime);

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