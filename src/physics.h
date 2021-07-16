#include "renderer.h"

#define GRAVITY -9.81f

#define MAX_RIGID_BODIES
#define MAX_STATIC_BODIES

// rigid body position = center of mass
struct Cube_Collider
{
	vec3 position;
	vec3 velocity;
	vec3 scale;
};

struct Sphere_Collider
{
	vec3 position;
	vec3 velocity;
	float radius;
};

struct Cylinder_Collider
{
	vec3 position;
	vec3 velocity;
	float radius;
	float height;
};

// axis-aligned colliders

struct Cube_Collider_AA
{
	vec3 min, max;
};

struct Cylinder_Collider_AA
{
	vec3 position;
	vec3 velocity;
	float radius;
	float height;
};

// static colliders
struct Plane_Collider_Static
{
	vec3 position;
	vec2 scale;
	vec3 normal;
};

struct Cube_Collider_Static
{
	vec3 position;
	vec3 scale;
	vec3 normal;
};

// --- collisions --- //

// point in shape
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
	if (point.y > cylinder.position.y + cylinder.height) return false;
	if (point.y < cylinder.position.y - cylinder.height) return false;

	float distance = (point.x - cylinder.position.x) * (point.x - cylinder.position.x) +
						  (point.z - cylinder.position.z) * (point.z - cylinder.position.z);

	return (distance < (cylinder.radius * cylinder.radius));
}

// shape in shape
bool sphere_sphere_intersect(Sphere_Collider sphere_1, Sphere_Collider sphere_2)
{
	float Distance = (sphere_1.position.x - sphere_2.position.x) * (sphere_1.position.x - sphere_2.position.x) +
		(sphere_1.position.y - sphere_2.position.y) * (sphere_1.position.y - sphere_2.position.y) +
		(sphere_1.position.z - sphere_2.position.z) * (sphere_1.position.z - sphere_2.position.z);

	float radius_sum = (sphere_1.radius * sphere_1.radius) + (sphere_2.radius * sphere_2.radius);

	return (Distance < radius_sum);
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
bool sphere_plane_intersect(Sphere_Collider sphere, Plane_Collider_Static plane)
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
bool cylinder_plane_intersect(Cylinder_Collider cylinder, Plane_Collider_Static plane)
{
	return false;
}

// --- core --- //

void init_colldier(Cube_Collider* cube, vec3 position, vec3 velocity, vec3 scale)
{
	cube->position = position;
	cube->velocity = velocity;
	cube->scale    = scale;
}
void init_collider(Sphere_Collider* sphere, vec3 position, vec3 velocity, float radius)
{
	sphere->position = position;
	sphere->velocity = velocity;
	sphere->radius   = radius;
}
void init_collider(Cylinder_Collider* cylinder, vec3 position, vec3 velocity, float height, float radius)
{
	cylinder->position = position;
	cylinder->velocity = velocity;
	cylinder->height   = height;
	cylinder->radius   = radius;
}

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
	ground = Plane_Collider_Static{ vec3(0,0,0), vec2(100,100), vec3(0,1,0) };
	wall   = Plane_Collider_Static{ vec3(5,0.5,6), vec2(1,1), vec3(0,0,-1) };

	if (sphere_plane_intersect(*collider, ground))
	{
		vec3 pos = vec3(collider->position.x, collider->position.y - collider->radius, collider->position.z);
		float dot_product = glm::dot(ground.normal, pos);
		collider->velocity.y = dot_product * -10.f; // assert(dot_product < 0);
	}
	else
	{
		collider->position.y += GRAVITY * dtime;
	}

	if (sphere_plane_intersect(*collider, wall))
	{
		vec3 pos = vec3(collider->position.x, collider->position.y - collider->radius, collider->position.z);
		float dot_product = glm::dot(wall.normal, pos);
		collider->velocity.z = dot_product * 1.f; // assert(dot_product < 0);
	}

	collider->velocity *= .85f; // damping
	collider->position += collider->velocity * dtime;
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

void init_collider(Plane_Collider_Static* plane, vec3 position, vec3 normal, vec3 scale)
{
	plane->position = position;
	plane->normal   = normal;
	plane->scale    = scale;
}

struct Physics_Colliders
{
	Cube_Collider      cubes[1];
	Sphere_Collider    spheres[1];
	Cylinder_Collider cylinders[1];
	Plane_Collider_Static planes[2];
};

void update_colliders(Physics_Colliders* colliders, float dtime)
{
	update_collider(colliders->cubes     , dtime);
	update_collider(colliders->spheres   , dtime);
	update_collider(colliders->cylinders, dtime);
}

// rendering

struct Cube_Collider_Drawable
{
	vec3 position;
	mat3 rotation;
};

struct Sphere_Collider_Drawable
{
	vec3 position;
	mat3 rotation;
};

struct Cylinder_Collider_Drawable
{
	vec3 position;
	mat3 rotation;
};

struct Plane_Collider_Drawable
{
	vec3 position;
	mat3 rotation;
};

struct Physics_Renderer
{
	Drawable_Mesh_UV cube_mesh, sphere_mesh, cylinder_mesh, plane_mesh;
	Shader shader;
};

void init(Physics_Renderer* renderer)
{
	load(&renderer->cube_mesh, "assets/meshes/cube.mesh_uv", "assets/textures/palette.bmp", sizeof(Cube_Collider_Drawable));
	mesh_add_attrib_vec3(3, sizeof(Cube_Collider_Drawable), 0); // world pos
	mesh_add_attrib_mat3(4, sizeof(Cube_Collider_Drawable), sizeof(vec3)); // rotation

	load(&renderer->sphere_mesh, "assets/meshes/sphere.mesh_uv", "assets/textures/palette.bmp", sizeof(Sphere_Collider_Drawable));
	mesh_add_attrib_vec3(3, sizeof(Sphere_Collider_Drawable), 0); // world pos
	mesh_add_attrib_mat3(4, sizeof(Sphere_Collider_Drawable), sizeof(vec3)); // rotation

	load(&renderer->cylinder_mesh, "assets/meshes/cylinder.mesh_uv", "assets/textures/palette.bmp", sizeof(Cylinder_Collider_Drawable));
	mesh_add_attrib_vec3(3, sizeof(Cylinder_Collider_Drawable), 0); // world pos
	mesh_add_attrib_mat3(4, sizeof(Cylinder_Collider_Drawable), sizeof(vec3)); // rotation

	load(&renderer->plane_mesh, "assets/meshes/plane.mesh_uv", "assets/textures/palette.bmp", sizeof(Plane_Collider_Drawable));
	mesh_add_attrib_vec3(3, sizeof(Plane_Collider_Drawable), 0); // world pos
	mesh_add_attrib_mat3(4, sizeof(Plane_Collider_Drawable), sizeof(vec3)); // rotation

	load(&(renderer->shader), "assets/shaders/mesh_uv_rot.vert", "assets/shaders/mesh_uv.frag");
	bind(renderer->shader);
	set_int(renderer->shader, "positions", 0);
	set_int(renderer->shader, "normals"  , 1);
	set_int(renderer->shader, "albedo"   , 2);
	set_int(renderer->shader, "texture_sampler", 3);
}
void update_renderer(Physics_Renderer* renderer, Physics_Colliders* colliders)
{
	Cube_Collider_Drawable cube = {};

	cube.position = colliders->cubes[0].position;
	cube.rotation = mat3(1.f);

	update(renderer->cube_mesh, sizeof(Cube_Collider_Drawable), (byte*)(&cube));

	Sphere_Collider_Drawable sphere = {};

	sphere.position = colliders->spheres[0].position;
	sphere.rotation = mat3(1.f);

	update(renderer->sphere_mesh, sizeof(Sphere_Collider_Drawable), (byte*)(&sphere));

	Cylinder_Collider_Drawable cylinder = {};

	cylinder.position = colliders->cylinders[0].position;
	cylinder.rotation = mat3(1.f);

	update(renderer->cylinder_mesh, sizeof(Cylinder_Collider_Drawable), (byte*)(&cylinder));

	Plane_Collider_Drawable plane = {};

	plane.position = colliders->planes[0].position;
	plane.rotation = point_at(colliders->planes[0].normal, vec3(0,1,0));

	update(renderer->plane_mesh, sizeof(Plane_Collider_Drawable), (byte*)(&plane));
}
void draw(Physics_Renderer* renderer, mat4 proj_view)
{
	bind(renderer->shader);
	set_mat4(renderer->shader, "proj_view", proj_view);

	bind_texture(renderer->cube_mesh     , 3);
	bind_texture(renderer->sphere_mesh   , 3);
	bind_texture(renderer->cylinder_mesh, 3);
	bind_texture(renderer->plane_mesh, 3);

	draw(renderer->cube_mesh     , 1);
	draw(renderer->sphere_mesh   , 1);
	draw(renderer->cylinder_mesh, 1);
	draw(renderer->plane_mesh, 1);
}