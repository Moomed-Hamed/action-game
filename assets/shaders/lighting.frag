#version 330 core

struct VS_OUT {
	vec2 tex_coords;
	vec3 view_pos;
};

struct Directional_Light {
    vec3 direction;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct Point_Light {
    vec3 position;
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

	float linear; // for attenuation (falloff)
	float quadratic;
};

struct Spot_Light {
	vec3  position;
	vec3  direction;
	float inner_cutoff; // used to fade away light
	float outer_cutoff; // max angle of the light

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float linear;
	float quadratic;
};

in VS_OUT vs_out;

// these come from the g_buffer
uniform sampler2D positions;
uniform sampler2D normals;
uniform sampler2D albedo;

uniform Directional_Light dir_light;
uniform Point_Light       pt_light;
uniform Spot_Light        spt_light;

layout (location = 0) out vec4 frag_color;

void main()
{
	vec3  position  = texture(positions, vs_out.tex_coords).rgb;
	vec3  normal    = texture(normals  , vs_out.tex_coords).rgb;
	vec3  albedo    = texture(albedo   , vs_out.tex_coords).rgb;
	float shininess = 8;

	// Directional Lights
	vec3 light_dir   = normalize(dir_light.direction) * -1;
	vec3 view_dir    = normalize(vs_out.view_pos - position);
	vec3 reflect_dir = reflect(light_dir * -1, normal);
	float spec = 0;//pow(max(dot(view_dir, reflect_dir), 0.0), 8);
	float diff = max(dot(normal, light_dir), 0.0);

	vec3 ambient  = dir_light.ambient  * albedo;
	vec3 diffuse  = dir_light.diffuse  * diff * albedo;
	vec3 specular = dir_light.specular * spec * albedo;
	vec3 final_color = .55 * (ambient + diffuse + specular); // This should be .33 probably

	// Point Lights
	light_dir   = normalize(pt_light.position - position);
	reflect_dir = reflect(light_dir * -1, normal);
	spec = pow(max(dot(view_dir, reflect_dir), 0.0), shininess);
	diff = max(dot(normal, light_dir), 0.0);

	float distance = length(pt_light.position - position);
	float attenuation = 1.0 / (1.0 + pt_light.linear * distance + pt_light.quadratic * (distance * distance));   
	
	ambient  += pt_light.ambient  * albedo;
	diffuse  += pt_light.diffuse  * diff * albedo;
	specular += pt_light.specular * spec * albedo;
	final_color += .33 * attenuation * (ambient + diffuse + specular);

	// Spot Lights
	light_dir   = normalize(spt_light.position - position);
	reflect_dir = reflect(light_dir * -1, normal);
	spec = pow(max(dot(view_dir, reflect_dir), 0.0), shininess);
	diff = max(dot(normal, light_dir), 0.0); 

	float theta     = dot(light_dir, normalize(-spt_light.direction));
	float epsilon   = spt_light.inner_cutoff - spt_light.outer_cutoff;// angle of fade reigion
	float intensity = clamp((theta - spt_light.outer_cutoff) / epsilon, 0.0, 1.0); 

	distance = length(spt_light.position - position);
	attenuation = 1.0 / (1.0 + spt_light.linear * distance + spt_light.quadratic * (distance * distance)); 

	ambient  += spt_light.ambient  *  albedo;
	diffuse  += spt_light.diffuse  * diff * albedo;
	specular += spt_light.specular * spec * albedo;
	final_color += .33 * intensity * attenuation * (ambient + diffuse + specular);

	frag_color = vec4(final_color, 1.0);
}