#include "physics.h"

#define MAX_WALLS  8

struct Floor_Tile
{
	Plane_Collider collider;
};

// rendering

struct Floor_Tile_Drawable
{
	vec3 position;
	mat3 rotation;
};

struct Tile_Renderer
{
	Floor_Tile_Drawable floors[1];

	Drawable_Mesh_UV floor_mesh, sky_mesh;
	Shader shader;
};

void init(Tile_Renderer* renderer)
{
	load(&renderer->floor_mesh, "assets/meshes/ground.mesh_uv", sizeof(Floor_Tile_Drawable));
	mesh_add_attrib_vec3(3, sizeof(Floor_Tile_Drawable), 0); // world pos
	mesh_add_attrib_mat3(4, sizeof(Floor_Tile_Drawable), sizeof(vec3)); // rotation

	load(&renderer->sky_mesh, "assets/meshes/sky.mesh_uv", sizeof(Floor_Tile_Drawable));
	mesh_add_attrib_vec3(3, sizeof(Floor_Tile_Drawable), 0); // world pos
	mesh_add_attrib_mat3(4, sizeof(Floor_Tile_Drawable), sizeof(vec3)); // rotation

	GLuint texture  = load_texture("assets/textures/palette2.bmp");
	GLuint material = load_texture("assets/textures/materials.bmp");

	renderer->floor_mesh.texture_id = texture;
	renderer->sky_mesh.texture_id   = texture;

	renderer->floor_mesh.material_id = material;
	renderer->sky_mesh.material_id   = material;

	load(&(renderer->shader), "assets/shaders/transform/mesh_uv.vert", "assets/shaders/mesh_uv.frag");
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

	bind_texture(renderer->floor_mesh);
	draw(renderer->floor_mesh);

	bind_texture(renderer->sky_mesh);
	draw(renderer->sky_mesh);
}

// orb rendering

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
	load(&renderer->mesh, "assets/meshes/orb.mesh_uv", sizeof(renderer->orbs));
	mesh_add_attrib_vec3(3, sizeof(Orb_Drawable), 0); // world pos
	mesh_add_attrib_mat3(4, sizeof(Orb_Drawable), sizeof(vec3)); // transform

	renderer->mesh.texture_id  = load_texture("assets/textures/palette2.bmp");
	renderer->mesh.material_id = load_texture("assets/textures/materials.bmp");

	load(&(renderer->shader), "assets/shaders/transform/mesh_uv.vert", "assets/shaders/mesh_uv.frag");
}
void update_renderer(Orb_Renderer* renderer, Orb* orbs)
{
	renderer->num_orbs = 0;

	for (uint i = 0; i < MAX_ORBS; i++)
	{
		if (orbs[i].type > 0)
		{
			vec3 axis = vec3(noise_chance(i), noise_chance(i + 1000), noise_chance(i + 2000));
			renderer->orbs[i].position = orbs[i].position;
			renderer->orbs[i].transform = mat3(1) * mat3(rotate(noise_chance(i) * TWOPI, axis));
			renderer->num_orbs++;
		} else renderer->orbs[i].transform = {};
	}

	update(renderer->mesh, renderer->num_orbs * sizeof(Orb_Drawable), (byte*)(renderer->orbs));
}
void draw(Orb_Renderer* renderer, mat4 proj_view)
{
	bind(renderer->shader);
	set_mat4(renderer->shader, "proj_view", proj_view);
	bind_texture(renderer->mesh);
	draw(renderer->mesh, renderer->num_orbs);
}

// sea rendering

struct Sea_Drawable
{
	vec3 position;
	vec3 color;
};

struct Sea_Renderer
{
	Sea_Drawable water_tiles[1]; // water tiles
	Drawable_Mesh mesh;
	Shader shader;
};

void init(Sea_Renderer* renderer)
{
	load(&renderer->mesh, "assets/meshes/sea.mesh", sizeof(renderer->water_tiles));
	mesh_add_attrib_vec3(2, sizeof(Sea_Drawable), 0 * sizeof(vec3)); // position
	mesh_add_attrib_vec3(3, sizeof(Sea_Drawable), 1 * sizeof(vec3)); // color
	load(&(renderer->shader), "assets/shaders/fluid.vert", "assets/shaders/mesh.frag");
}
void update_renderer(Sea_Renderer* renderer, vec3 pos)
{
	renderer->water_tiles[0].position = {};//  pos * vec3(1, 0, 1);
	renderer->water_tiles[0].color     = vec3(0, 0.203, 0.254);

	update(renderer->mesh, sizeof(renderer->water_tiles), (byte*)(&renderer->water_tiles));
}
void draw(Sea_Renderer* renderer, mat4 proj_view, float dtime)
{
	static float timer = 0; timer += dtime / 3;
	if (timer > TWOPI) timer = 0;


	bind(renderer->shader);
	set_mat4(renderer->shader, "proj_view", proj_view);
	set_float(renderer->shader, "timer", timer);
	draw(renderer->mesh, 1);
}

// prop renderer

#define MAX_PROPS 8 // max number of props of a given kind
#define NUM_PROP_TYPES 4

#define PROP_STATIC 1
#define PROP_DESTRUCTIBLE 2

struct Prop
{
	uint type;
	vec3 position;
	mat3 transform;
};

struct Props
{
	union
	{
		Prop props[MAX_PROPS * NUM_PROP_TYPES];

		struct
		{
			Prop trees[MAX_PROPS];
			Prop crates[MAX_PROPS];
			Prop barrels[MAX_PROPS];
			Prop campfires[MAX_PROPS];
		};
	};
};

void init(Props* props)
{
	props->trees[0].type = PROP_STATIC;
	props->trees[0].position  = vec3(5, 0, -5);
	props->trees[0].transform = mat3(.5);

	props->campfires[0].type = PROP_STATIC;
	props->campfires[0].position  = vec3(5, 0, 0);
	props->campfires[0].transform = mat3(1);

	props->crates[0].type = PROP_STATIC;
	props->crates[0].position  = vec3(-5, 0, 0);
	props->crates[0].transform = mat3(1);

	props->barrels[0].type = PROP_STATIC;
	props->barrels[0].position  = vec3(-2, 0, 6);
	props->barrels[0].transform = mat3(1);
}
void update(Props* props)
{

}

// rendering

struct Prop_Drawable
{
	vec3 position;
	mat3 transform;
};

struct Prop_Renderer
{
	Prop_Drawable props[MAX_PROPS];
	GLuint texture_id, material_id;
	Drawable_Mesh_UV meshes[4];
	Shader shader;
};

void init(Prop_Renderer* renderer)
{
	load(&(renderer->shader), "assets/shaders/transform/mesh_uv.vert", "assets/shaders/mesh_uv.frag");

	renderer->texture_id  = load_texture("assets/textures/palette2.bmp");
	renderer->material_id = load_texture("assets/textures/materials.bmp");

	load(&renderer->meshes[0], "assets/meshes/tree.mesh_uv", MAX_PROPS * sizeof(Prop_Drawable));
	mesh_add_attrib_vec3(3, sizeof(Prop_Drawable), 0 * sizeof(vec3)); // position
	mesh_add_attrib_mat3(4, sizeof(Prop_Drawable), 1 * sizeof(vec3)); // color

	load(&renderer->meshes[1], "assets/meshes/crate.mesh_uv", MAX_PROPS * sizeof(Prop_Drawable));
	mesh_add_attrib_vec3(3, sizeof(Prop_Drawable), 0 * sizeof(vec3)); // position
	mesh_add_attrib_mat3(4, sizeof(Prop_Drawable), 1 * sizeof(vec3)); // color

	load(&renderer->meshes[2], "assets/meshes/barrel.mesh_uv", MAX_PROPS * sizeof(Prop_Drawable));
	mesh_add_attrib_vec3(3, sizeof(Prop_Drawable), 0 * sizeof(vec3)); // position
	mesh_add_attrib_mat3(4, sizeof(Prop_Drawable), 1 * sizeof(vec3)); // color

	load(&renderer->meshes[3], "assets/meshes/campfire.mesh_uv", MAX_PROPS * sizeof(Prop_Drawable));
	mesh_add_attrib_vec3(3, sizeof(Prop_Drawable), 0 * sizeof(vec3)); // position
	mesh_add_attrib_mat3(4, sizeof(Prop_Drawable), 1 * sizeof(vec3)); // color
}
void update_renderer(Prop_Renderer* renderer, Props* props)
{
	for (uint i = 0; i < NUM_PROP_TYPES; i++)
	{
		for (uint j = 0; j < MAX_PROPS; j++)
		{
			uint index = (i * MAX_PROPS) + j;
			if (props->props[index].type > 0)
			{
				renderer->props[j].position  = props->props[index].position;
				renderer->props[j].transform = props->props[index].transform;
			} else renderer->props[j] = {};
		}

		update(renderer->meshes[i], sizeof(renderer->props), (byte*)renderer->props);
	}
}
void draw(Prop_Renderer* renderer, mat4 proj_view)
{
	bind(renderer->shader);
	set_mat4(renderer->shader, "proj_view", proj_view);

	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, renderer->texture_id);
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, renderer->material_id);

	for (uint i = 0; i < NUM_PROP_TYPES; i++)
	{
		draw(renderer->meshes[i], MAX_PROPS);
	}
}