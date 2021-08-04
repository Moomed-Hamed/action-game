#include "physics.h"

#define MAX_WALLS  8

struct Floor_Tile
{
	Plane_Collider collider;
};

struct Wall_Tile
{
	Plane_Collider collider;
};

// rendering

struct Floor_Tile_Drawable
{
	vec3 position;
	mat3 rotation;
};

struct Wall_Tile_Drawable
{
	vec3 position;
	mat3 rotation;
};

struct Tile_Renderer
{
	Floor_Tile_Drawable floors[1];
	Wall_Tile_Drawable  walls[1];

	Drawable_Mesh_UV floor_mesh, sky_mesh, wall_mesh;
	Shader shader;
};

void init(Tile_Renderer* renderer)
{
	load(&renderer->floor_mesh, "assets/meshes/ground.mesh_uv", "assets/textures/palette.bmp", sizeof(Floor_Tile_Drawable));
	mesh_add_attrib_vec3(3, sizeof(Floor_Tile_Drawable), 0); // world pos
	mesh_add_attrib_mat3(4, sizeof(Floor_Tile_Drawable), sizeof(vec3)); // rotation

	load(&renderer->sky_mesh, "assets/meshes/sky.mesh_uv", "assets/textures/palette.bmp", sizeof(Floor_Tile_Drawable));
	mesh_add_attrib_vec3(3, sizeof(Floor_Tile_Drawable), 0); // world pos
	mesh_add_attrib_mat3(4, sizeof(Floor_Tile_Drawable), sizeof(vec3)); // rotation

	load(&renderer->wall_mesh, "assets/meshes/floor.mesh_uv", "assets/textures/palette.bmp", sizeof(Wall_Tile_Drawable));
	mesh_add_attrib_vec3(3, sizeof(Wall_Tile_Drawable), 0); // world pos
	mesh_add_attrib_mat3(4, sizeof(Wall_Tile_Drawable), sizeof(vec3)); // rotation

	load(&(renderer->shader), "assets/shaders/mesh_uv_rot.vert", "assets/shaders/mesh_uv.frag");
	bind(renderer->shader);
	set_int(renderer->shader, "positions", 0);
	set_int(renderer->shader, "normals"  , 1);
	set_int(renderer->shader, "albedo"   , 2);
	set_int(renderer->shader, "texture_sampler", 3);
}
void update_renderer(Tile_Renderer* renderer)
{
	Floor_Tile_Drawable tile = {};

	tile.position = vec3(0, 0, 0);
	tile.rotation = mat3(1.f);

	update(renderer->floor_mesh, sizeof(Floor_Tile_Drawable), (byte*)(&tile));
	update(renderer->sky_mesh  , sizeof(Floor_Tile_Drawable), (byte*)(&tile));
}
void draw(Tile_Renderer* renderer, mat4 proj_view)
{
	bind(renderer->shader);
	set_mat4(renderer->shader, "proj_view", proj_view);

	bind_texture(renderer->floor_mesh, 3);
	draw(renderer->floor_mesh, 1);

	bind_texture(renderer->sky_mesh, 3);
	draw(renderer->sky_mesh, 1);
}

// ---

struct Prop_Barrel
{
	Cylinder_Collider collider;
};

void init(Prop_Barrel* barrels)
{

}
void update(Prop_Barrel* barrels)
{

}

struct Prop_Crate
{
	Cylinder_Collider collider;
};

void init(Prop_Crate* barrels)
{

}
void update(Prop_Crate* barrels)
{

}

#define MAX_ORBS 16

// orbs (xp, health, etc.)
struct Orb
{
	uint type;
	vec3 position;
	vec3 velocity;
};

void spawn(Orb* orbs, vec3 position)
{
	for (uint i = 0; i < MAX_ORBS; i++)
	{
		if (orbs[i].type == 0)
		{
			orbs[i] = {};
			orbs[i].type = 1;
			orbs[i].position = position;
			orbs[i].velocity = vec3(random_chance(), random_chance(), random_chance()) * 10.f;
			return;
		}
	}
}
void update(Orb* orbs, vec3 player_pos, float dtime, Audio sound)
{
	for (uint i = 0; i < MAX_ORBS; i++)
	{
		if (orbs[i].type > 0)
		{
			vec3 target_velocity = player_pos - orbs[i].position; // move towards the player
			if (length(target_velocity) < 1 || length((player_pos - vec3(0, 1.5, 0)) - orbs[i].position) < 1)
			{
				play_audio(sound); // orb has been absorbed
				orbs[i] = {};
				orbs[i].velocity = vec3(random_chance(), random_chance(), random_chance()) * 10.f;
			}
			orbs[i].velocity = lerp(orbs[i].velocity, target_velocity * 2.f, dtime * 2);
			orbs[i].position += orbs[i].velocity * dtime;
		}
	}
}

// rendering

struct Orb_Drawable
{
	vec3 position;
	mat3 transform;
	//vec2 texture_offset;
};

struct Orb_Renderer
{
	uint num_orbs;
	Orb_Drawable orbs[MAX_ORBS];
	Drawable_Mesh_UV mesh;
	Shader shader;
};

void init(Orb_Renderer* renderer)
{
	load(&renderer->mesh, "assets/meshes/orb.mesh_uv", "assets/textures/palette.bmp", MAX_ORBS * sizeof(Orb_Drawable));
	mesh_add_attrib_vec3(3, sizeof(Orb_Drawable), 0); // world pos
	mesh_add_attrib_mat3(4, sizeof(Orb_Drawable), sizeof(vec3)); // transform

	load(&(renderer->shader), "assets/shaders/mesh_uv_rot.vert", "assets/shaders/mesh_uv.frag");
	bind(renderer->shader);
	set_int(renderer->shader, "positions", 0);
	set_int(renderer->shader, "normals"  , 1);
	set_int(renderer->shader, "albedo"   , 2);
	set_int(renderer->shader, "texture_sampler", 3);
}
void update_renderer(Orb_Renderer* renderer, Orb* orbs)
{
	renderer->num_orbs = 0;

	for (uint i = 0; i < MAX_ORBS; i++)
	{
		renderer->num_orbs += 1;
		renderer->orbs[i].position = orbs[i].position;
		//static int n = 0;
		//quat q = normalize(quat(noise_chance(n), noise_chance(n + 1), noise_chance(n + 2), noise_chance(n++ + 3)));

		renderer->orbs[i].transform = mat3(.6);
	}

	update(renderer->mesh, renderer->num_orbs * sizeof(Orb_Drawable), (byte*)(renderer->orbs));
}
void draw(Orb_Renderer* renderer, mat4 proj_view)
{
	bind(renderer->shader);
	set_mat4(renderer->shader, "proj_view", proj_view);
	bind_texture(renderer->mesh, 3);
	draw(renderer->mesh, renderer->num_orbs);
}

// destructible walls