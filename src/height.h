#include "physics.h"

#define HEIGHTMAP_N	1024
#define HEIGHTMAP_L	256
#define HEIGHTMAP_S	20

struct Heightmap
{
	float height[HEIGHTMAP_N * HEIGHTMAP_N];
};

float height(Heightmap* map, vec3 pos)
{
	uint x = (pos.x / HEIGHTMAP_L) * HEIGHTMAP_N;
	uint z = (pos.z / HEIGHTMAP_L) * HEIGHTMAP_N;

	return map->height[(x * HEIGHTMAP_N) + z] * HEIGHTMAP_S;
}

float height(Heightmap* map, vec2 pos)
{
	uint x = (pos.x / HEIGHTMAP_L) * HEIGHTMAP_N;
	uint y = (pos.y / HEIGHTMAP_L) * HEIGHTMAP_N;

	return map->height[(x * HEIGHTMAP_N) + y] * HEIGHTMAP_S;
}

vec3 terrain(Heightmap* map, vec2 pos)
{
	uint x = (pos.x / HEIGHTMAP_L) * HEIGHTMAP_N;
	uint z = (pos.y / HEIGHTMAP_L) * HEIGHTMAP_N;

	return vec3(pos.x, map->height[(x * HEIGHTMAP_N) + z] * HEIGHTMAP_S, pos.y);
}

struct Heightmap_Renderer
{
	Drawable_Mesh mesh;
	Shader shader;
	GLuint heights, normals, albedo;
};

void init(Heightmap_Renderer* renderer, Heightmap* heightmap, const char* path)
{
	load(&renderer->mesh, "assets/meshes/env/terrain.mesh", 0);
	load(&(renderer->shader), "assets/shaders/terrain.vert", "assets/shaders/terrain.frag");

	load_file_r32(path, heightmap->height, HEIGHTMAP_N);

	glGenTextures(1, &renderer->heights);
	glBindTexture(GL_TEXTURE_2D, renderer->heights);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, HEIGHTMAP_N, HEIGHTMAP_N, 0, GL_RED, GL_FLOAT, heightmap->height);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	renderer->normals = load_texture_png("assets/textures/normals.png");
	renderer->albedo  = load_texture_png("assets/textures/albedo.png");
}
void draw(Heightmap_Renderer* renderer, mat4 proj_view)
{
	bind(renderer->shader);
	set_mat4(renderer->shader, "proj_view", proj_view);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, renderer->heights);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, renderer->normals);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, renderer->albedo);

	draw(renderer->mesh);
}