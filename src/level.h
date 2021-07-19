#include "player.h"

struct Level
{
	// players
	Player player;
	Enemy enemy;

	// gameplay
	Bullet bullets[MAX_BULLETS];

	// static geometry
	Plane_Collider walls[16], floors[16];

	// props & dynamic physics objects
	//Barrel barrels[MAX_BARRELS];
};

void init(Level* level)
{
	*level = {};

	init(&level->player);
	init(&level->enemy);
}
void update_level(Level* level, float dtime)
{
	update_bullets(level->bullets, dtime);
	update(&level->enemy, dtime);

	for (int i = 0; i < MAX_BULLETS; i++) // bullet collisions
	{
		if (point_in_cylinder(level->bullets[i].position, level->enemy.hitbox))
		{
			level->enemy.feet.velocity += 1;
		}
	}

	level->enemy.feet.position.y += GRAVITY * dtime;
}