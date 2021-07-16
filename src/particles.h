#include "physics.h"

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