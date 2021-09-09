#version 420 core

struct VS_OUT
{
	vec3 normal;
	vec3 frag_pos;
	vec3 color;
};

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 world_position;
layout (location = 3) in vec3 color;

uniform mat4 proj_view;
uniform float timer;

layout (binding = 0) uniform sampler2D heightmap;
layout (binding = 1) uniform sampler2D normalmap;
layout (binding = 2) uniform sampler2D foammap;

out VS_OUT vs_out;

void main()
{
	vs_out.frag_pos = position + world_position + texture(heightmap, position.xz / 50).rgb;
	vs_out.normal   = texture(normalmap, position.xz / 50).rgb;

	//float foam = texture(foammap, position.xz / 50).r;
	//if(foam > .9) vs_out.color    = vec3(1,1,1);
	//else vs_out.color    = vec3(0,0,1);

	vs_out.color    = vec3(0,0,1);

	vs_out.normal   = vec3 ( 0,1,0);//normalize(texture(normalmap, position.xz / 10).rgb);
	vs_out.frag_pos = position + world_position + (3*texture(heightmap, position.xz / 10).rgb);
	vs_out.color    = color * (10 * texture(foammap, position.xz / 10).rgb + .1);

	//if(texture(foammap, position.xz / 10).r > .9) vs_out.color = vec3(1);

	gl_Position = proj_view * vec4(vs_out.frag_pos, 1.0);
}