#include "physics.h"

#define MAX_FLOORS 8
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

	Drawable_Mesh_UV floor_mesh, wall_mesh;
	Shader shader;
};

void init(Tile_Renderer* renderer)
{
	load(&renderer->floor_mesh, "assets/meshes/ground.mesh_uv", "assets/textures/palette.bmp", sizeof(Floor_Tile_Drawable));
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
}
void draw(Tile_Renderer* renderer, mat4 proj_view)
{
	bind(renderer->shader);
	set_mat4(renderer->shader, "proj_view", proj_view);
	bind_texture(renderer->floor_mesh, 3);
	draw(renderer->floor_mesh, 1);
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

// pickups (health, ammo, etc.)
struct Pickup
{
	uint type;
	vec3 position;
};

// orbs (xp, health, etc.)
struct Orb
{
	uint type;
	vec3 position;
	vec3 velocity;
};

// destructible walls