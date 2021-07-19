#include "renderer.h"

#define GRAVITY -9.80665f

#define MAX_CUBE_COLLIDERS      8
#define MAX_PLANE_COLLIDERS     8
#define MAX_SPHERE_COLLIDERS    8
#define MAX_CYLINDER_COLLIDERS  8

struct Cube_Collider // rigid body position = center of mass
{
	vec3 position, velocity, force;
	float mass;
	vec3 scale;
};

struct Sphere_Collider
{
	vec3 position, velocity, force;
	float mass;
	float radius;
};

struct Cylinder_Collider
{
	vec3 position, velocity, force;
	float mass;
	float radius;
	float height;
};

// AA = axis-aligned
struct Cube_Collider_AA
{
	vec3 position, velocity, force;
	vec3 min, max;
	float mass;
};

struct Cylinder_Collider_AA
{
	vec3 position, velocity, force;
	float radius;
	float height;
};

struct Plane_Collider // _AA? _Static?
{
	vec3 position, velocity, force;
	vec3 normal;
	vec2 scale;
};

// --- collisions --- //

bool point_in_cube_aa(vec3 point, Cube_Collider_AA cube)
{
	if (point.x < cube.min.x || point.x > cube.max.x) return false;
	if (point.z < cube.min.z || point.z > cube.max.z) return false;
	if (point.y < cube.min.y || point.y > cube.max.y) return false;
	return true;
}
bool point_in_sphere(vec3 point, Sphere_Collider sphere)
{
	float distance = (point.x - sphere.position.x) * (point.x - sphere.position.x) +
		              (point.y - sphere.position.y) * (point.y - sphere.position.y) +
		              (point.z - sphere.position.z) * (point.z - sphere.position.z);

	return (distance < (sphere.radius * sphere.radius));
}
bool point_in_cylinder(vec3 point, Cylinder_Collider cylinder)
{
	if (point.y > cylinder.position.y + (cylinder.height / 2.f)) return false;
	if (point.y < cylinder.position.y - (cylinder.height / 2.f)) return false;

	float distance = (point.x - cylinder.position.x) * (point.x - cylinder.position.x) +
						  (point.z - cylinder.position.z) * (point.z - cylinder.position.z);

	return (distance < (cylinder.radius * cylinder.radius));
}

bool sphere_cube_aa_intersect(Sphere_Collider sphere, Cube_Collider_AA cube)
{
	float x_pos, y_pos, z_pos;

	if (sphere.position.x < cube.min.x) x_pos = cube.min.x;
	else if (sphere.position.x > cube.max.x) x_pos = cube.max.x;
	else x_pos = sphere.position.x;

	if (sphere.position.y < cube.min.y) y_pos = cube.min.y;
	else if (sphere.position.y > cube.max.y) y_pos = cube.max.y;
	else y_pos = sphere.position.y;

	if (sphere.position.z < cube.min.z) z_pos = cube.min.z;
	else if (sphere.position.z > cube.max.z) z_pos = cube.max.z;
	else z_pos = sphere.position.z;

	return point_in_sphere(vec3(x_pos, y_pos, z_pos), sphere);
}
bool sphere_sphere_intersect(Sphere_Collider sphere_1, Sphere_Collider sphere_2)
{
	float distance = (sphere_1.position.x - sphere_2.position.x) * (sphere_1.position.x - sphere_2.position.x) +
		(sphere_1.position.y - sphere_2.position.y) * (sphere_1.position.y - sphere_2.position.y) +
		(sphere_1.position.z - sphere_2.position.z) * (sphere_1.position.z - sphere_2.position.z);

	float radius_sum = (sphere_1.radius * sphere_1.radius) + (sphere_2.radius * sphere_2.radius);

	return (distance < radius_sum);
}
bool sphere_plane_intersect(Sphere_Collider sphere, Plane_Collider plane)
{
	vec3 n = plane.normal;
	vec3 p = plane.position;
	vec3 s = sphere.position;

	float epsilon = (n.x * (s.x - p.x)) + (n.y * (s.y - p.y)) + (n.z * (s.z - p.z));
	if (epsilon > sphere.radius) return false;
	
	if (abs(dot(n, vec3(0, 1, 0))) - 1> 0.01)
	{
		mat3 undo_rotation = inverse(point_at(n, vec3(0, 1, 0))); // only works on vertical walls (fix 'up')

		// assert(n * undo_rotation == vec3(0, 1, 0);
		s = undo_rotation * s;
		p = undo_rotation * p;
	}

	if (s.x > (plane.scale.x /  2.f) + p.x) return false; // these don't take radius into account
	if (s.z > (plane.scale.y /  2.f) + p.z) return false; // these don't take radius into account
	if (s.x < (plane.scale.x / -2.f) + p.x) return false; // these don't take radius into account
	if (s.z < (plane.scale.y / -2.f) + p.z) return false; // these don't take radius into account

	return true;
}

// --- core --- //

void init_colldier(Cube_Collider* cube, vec3 position, vec3 velocity, vec3 force, float mass, vec3 scale)
{
	cube->position = position;
	cube->velocity = velocity;
	cube->force    = force;
	cube->mass     = mass;
	cube->scale    = scale;
}
void init_collider(Plane_Collider* plane, vec3 position, vec3 velocity, vec3 force, vec3 normal, vec2 scale)
{
	plane->position = position;
	plane->velocity = velocity;
	plane->force    = force;
	plane->normal   = normal;
	plane->scale    = scale;
}
void init_collider(Sphere_Collider* sphere, vec3 position, vec3 velocity, vec3 force, float mass, float radius)
{
	sphere->position = position;
	sphere->velocity = velocity;
	sphere->force    = force;
	sphere->mass     = mass;
	sphere->radius   = radius;
}
void init_collider(Cylinder_Collider* cylinder, vec3 position, vec3 velocity, vec3 force, float mass, float height, float radius)
{
	cylinder->position = position;
	cylinder->velocity = velocity;
	cylinder->force    = force;
	cylinder->mass     = mass;
	cylinder->height   = height;
	cylinder->radius   = radius;
}

struct Dynamic_Colliders
{
	Cube_Collider     cubes[MAX_CUBE_COLLIDERS];
	Sphere_Collider   spheres[MAX_SPHERE_COLLIDERS];
	Cylinder_Collider cylinders[MAX_CYLINDER_COLLIDERS];
};

struct Fixed_Colliders // static is a reserved keyword lol
{
	Cube_Collider  cubes[MAX_CUBE_COLLIDERS];
	Plane_Collider planes[MAX_PLANE_COLLIDERS];
};

struct Physics_Colliders
{
	Dynamic_Colliders dynamic;
	Fixed_Colliders   fixed;
};

// rendering

struct Collider_Drawable
{
	vec3 position;
	mat3 transform; // rotation & scale
};

struct Physics_Renderer
{
	uint num_planes, num_cubes, num_spheres, num_cylinders;

	Collider_Drawable planes   [MAX_PLANE_COLLIDERS];
	Collider_Drawable cubes    [MAX_CUBE_COLLIDERS     * 2]; // for dynamic & fixed
	Collider_Drawable spheres  [MAX_SPHERE_COLLIDERS   * 2]; // for dynamic & fixed
	Collider_Drawable cylinders[MAX_CYLINDER_COLLIDERS * 2]; // for dynamic & fixed

	Drawable_Mesh_UV cube_mesh, sphere_mesh, cylinder_mesh, plane_mesh;
	Shader shader;
};

void init(Physics_Renderer* renderer)
{
	uint reserved_size = sizeof(Collider_Drawable) * MAX_PLANE_COLLIDERS;

	load(&renderer->cube_mesh, "assets/meshes/cube.mesh_uv", "assets/textures/palette.bmp", reserved_size);
	mesh_add_attrib_vec3(3, sizeof(Collider_Drawable), 0); // world pos
	mesh_add_attrib_mat3(4, sizeof(Collider_Drawable), sizeof(vec3)); // transform

	load(&renderer->sphere_mesh, "assets/meshes/sphere.mesh_uv", "assets/textures/palette.bmp", reserved_size);
	mesh_add_attrib_vec3(3, sizeof(Collider_Drawable), 0); // world pos
	mesh_add_attrib_mat3(4, sizeof(Collider_Drawable), sizeof(vec3)); // transform

	load(&renderer->cylinder_mesh, "assets/meshes/cylinder.mesh_uv", "assets/textures/palette.bmp", reserved_size);
	mesh_add_attrib_vec3(3, sizeof(Collider_Drawable), 0); // world pos
	mesh_add_attrib_mat3(4, sizeof(Collider_Drawable), sizeof(vec3)); // transform

	load(&renderer->plane_mesh, "assets/meshes/plane.mesh_uv", "assets/textures/palette.bmp", reserved_size);
	mesh_add_attrib_vec3(3, sizeof(Collider_Drawable), 0); // world pos
	mesh_add_attrib_mat3(4, sizeof(Collider_Drawable), sizeof(vec3)); // transform

	load(&(renderer->shader), "assets/shaders/mesh_uv_rot.vert", "assets/shaders/mesh_uv.frag");
	bind(renderer->shader);
	set_int(renderer->shader, "positions", 0);
	set_int(renderer->shader, "normals"  , 1);
	set_int(renderer->shader, "albedo"   , 2);
	set_int(renderer->shader, "texture_sampler", 3);
}
void update_renderer(Physics_Renderer* renderer, Physics_Colliders* colliders)
{
	renderer->num_cubes     = 0;
	renderer->num_planes    = 0;
	renderer->num_spheres   = 0;
	renderer->num_cylinders = 0;

	for (uint i = 0; i < MAX_CUBE_COLLIDERS; i++)
	{
		if (colliders->dynamic.cubes[i].scale.x > 0) // is there a better way to check?
		{
			renderer->num_cubes += 1;
			renderer->cubes[i].position = colliders->dynamic.cubes[i].position;
			renderer->cubes[i].transform = mat3(1.f);
		}
	}

	for (uint i = 0; i < MAX_PLANE_COLLIDERS; i++)
	{
		if (colliders->fixed.planes[i].scale.x > 0)
		{
			renderer->num_planes += 1;
			renderer->planes[i].position = colliders->fixed.planes[i].position;
			renderer->planes[i].transform = point_at(colliders->fixed.planes[i].normal, vec3(1, 0, 0)); // ???
		}
	}

	for (uint i = 0; i < MAX_SPHERE_COLLIDERS; i++)
	{
		if (colliders->dynamic.spheres[i].radius > 0)
		{
			renderer->num_spheres += 1;
			renderer->spheres[i].position = colliders->dynamic.spheres[i].position;
			renderer->spheres[i].transform = mat3(1);
		}
	}

	for (uint i = 0; i < MAX_CYLINDER_COLLIDERS; i++)
	{
		if (colliders->dynamic.cylinders[i].radius > 0)
		{
			renderer->num_cylinders += 1;
			renderer->cylinders[i].position = colliders->dynamic.cylinders[i].position;
			renderer->cylinders[i].transform = mat3(1);
		}
	}

	update(renderer->cube_mesh, sizeof(Collider_Drawable) * renderer->num_cubes, (byte*)(&renderer->cubes));
	update(renderer->plane_mesh, sizeof(Collider_Drawable) * renderer->num_planes, (byte*)(&renderer->planes));
	update(renderer->sphere_mesh, sizeof(Collider_Drawable) * renderer->num_spheres, (byte*)(&renderer->spheres));
	update(renderer->cylinder_mesh, sizeof(Collider_Drawable) * renderer->num_cylinders, (byte*)(&renderer->cylinders));
}
void draw(Physics_Renderer* renderer, mat4 proj_view)
{
	bind(renderer->shader);
	set_mat4(renderer->shader, "proj_view", proj_view);

	bind_texture(renderer->cube_mesh    , 3);
	bind_texture(renderer->plane_mesh   , 3);
	bind_texture(renderer->sphere_mesh  , 3);
	bind_texture(renderer->cylinder_mesh, 3);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//glDisable(GL_CULL_FACE);
	draw(renderer->cube_mesh    , renderer->num_cubes);
	draw(renderer->plane_mesh   , renderer->num_planes);
	draw(renderer->sphere_mesh  , renderer->num_spheres);
	draw(renderer->cylinder_mesh, renderer->num_cylinders);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_CULL_FACE);
}

// particle fx

#define MAX_PARTICLES 16

struct Particle
{
	vec3 position;
	vec3 velocity;
};

struct Particle_Emitter
{
	Particle particles[MAX_PARTICLES];
};

/*
void update_collider(Sphere_Collider* collider, float dtime)
{
	Plane_Collider ground = {};

	collider->force.y += GRAVITY * dtime;

	if (sphere_plane_intersect(*collider, ground))
	{
		float penetration_depth = 0;

		collider->position.y -= GRAVITY * dtime;

		vec3 pos = vec3(collider->position.x, collider->position.y - collider->radius, collider->position.z);
		float dot_product = glm::dot(vec3(0, 1, 0), pos); //out(dot_product);
		collider->velocity.y = dot_product * -10; // assert(dot_product < 0);
	}

	collider->velocity += (collider->force / collider->mass) * dtime;
	collider->position += collider->velocity * dtime;
	// collider->velocity *= .9f; // damping
} */

/*
void update_collider(Cube_Collider* collider, float dtime)
{
	vec3 pos = vec3(collider->position.x, collider->position.y - (collider->scale.y / 2.f), collider->position.z);
	float dot_product = glm::dot(vec3(0, 1, 0), pos);

	if (dot_product < 0)
	{
		collider->velocity.y = dot_product * -10.f;
	}
	else
	{
		collider->position.y += GRAVITY * dtime;
	}

	collider->velocity *= .85f; // damping
	collider->position += collider->velocity * dtime;
}
void update_collider(Sphere_Collider* collider, float dtime)
{
	Plane_Collider_Static ground, wall;
	ground = Plane_Collider_Static{ vec3(0, 0, 0), vec2(100, 100), vec3(0, 1, 0) };
	wall   = Plane_Collider_Static{ vec3(5,0.5,6), vec2(1,1), vec3(0,0,-1) };

	collider->position.y += GRAVITY * dtime;

	if (sphere_plane_intersect(*collider, ground))
	{
		collider->position.y -= GRAVITY * dtime;

		vec3 pos = vec3(collider->position.x, collider->position.y - collider->radius, collider->position.z);
		float dot_product = glm::dot(vec3(0, 1, 0), pos); //out(dot_product);
		collider->velocity.y = dot_product * -10; // assert(dot_product < 0);
	}

	if (sphere_plane_intersect(*collider, wall))
	{
		vec3 pos = vec3(collider->position.x, collider->position.y - collider->radius, collider->position.z);
		float dot_product = glm::dot(wall.normal, pos);
		collider->velocity.z = dot_product * 1.f; // assert(dot_product < 0);
	}

	collider->position += collider->velocity * dtime;
	collider->velocity *= .9f; // damping
}
void update_collider(Cylinder_Collider* collider, float dtime)
{
	vec3 pos = vec3(collider->position.x, collider->position.y - (collider->height / 2), collider->position.z);
	float dot_product = glm::dot(vec3(0, 1, 0), pos);

	if (dot_product < 0)
	{
		collider->velocity.y = dot_product * -10.f;
	}
	else
	{
		collider->position.y += GRAVITY * dtime;
	}

	collider->velocity *= .85f; // damping
	collider->position += collider->velocity * dtime;
}
*/