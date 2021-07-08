#version 330 core

struct VS_OUT
{
	vec3 normal;   // normal vector
	vec3 frag_pos; // position of this pixel in world space
	vec2 tex_coord;
};

in VS_OUT vs_out;

layout (location = 0) out vec4 frag_position;
layout (location = 1) out vec4 frag_normal;
layout (location = 2) out vec4 frag_albedo;

uniform sampler2D texture_sampler;

void main()
{
	frag_position = vec4(vs_out.frag_pos, 0);
	frag_normal   = vec4(vs_out.normal, 0);
	frag_albedo   = vec4(texture(texture_sampler, vs_out.tex_coord).rgb, 1);
}