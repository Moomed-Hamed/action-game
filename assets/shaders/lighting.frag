#version 330 core

struct VS_OUT
{
	vec2 tex_coords;
	vec3 view_pos;
};

in VS_OUT vs_out;

// these come from the g_buffer
uniform sampler2D positions;
uniform sampler2D normals;
uniform sampler2D albedo;

layout (location = 0) out vec4 frag_color;

uniform float metallic;
uniform float roughness;
uniform float ao;

// lights
uniform vec3 light_positions[4];
uniform vec3 light_colors[4];

const float PI = 3.14159265359;

float distribution_GGX(float n_dot_h, float roughness);
float geometry_chlick_GGX(float n_dot_v, float roughness);
float geometry_smith(float n_dot_v, float n_dot_l, float roughness);
vec3  fresnel_schlick(float cosTheta, vec3 F0);

void main()
{
	vec3  position  = texture(positions, vs_out.tex_coords).rgb;
	vec3  normal    = texture(normals  , vs_out.tex_coords).rgb;
	vec3  albedo    = texture(albedo   , vs_out.tex_coords).rgb;

	vec3 N = normalize(normal);
	vec3 V = normalize(vs_out.view_pos - position); // world_pos -> camera

	// calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
	// of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
	vec3 base_reflectivity = mix(vec3(0.04), albedo, metallic);

	// reflectance equation
	vec3 Lo = vec3(0.0); // Lo = outgoing radiance

	// directional light
	{
		vec3 light_dir = normalize(vec3(-1));
		vec3 light_color = vec3(1, 0.749, 0.666);

		// per-light radiance
		vec3 L = normalize(light_dir * -1);
		vec3 H = normalize(V + L);
		vec3 radiance = light_color * .2;

		float n_dot_v = max(dot(N, V), 0.0000001); // prevent divide by 0
		float n_dot_l = max(dot(N, L), 0.0000001); // prevent divide by 0
		float h_dot_v = dot(H, V);
		float n_dot_h = dot(N, H);

		// Cook-Torrance BRDF
		float NDF = distribution_GGX(n_dot_h, roughness);
		float G   = geometry_smith (n_dot_v, n_dot_l, roughness);
		vec3  F   = fresnel_schlick(h_dot_v, base_reflectivity);
           
		vec3 specular = (NDF * G * F) / (4 * n_dot_v * n_dot_l);
		vec3 diffuse  = (vec3(1.0) - F) * (1.0 - metallic);

		// note: we multiplied the BRDF by F (in diffuse calculation) so we won't do it again here
		Lo += (diffuse * (albedo / PI) + specular) * radiance * n_dot_l;
	}

	for(int i = 0; i < 4; ++i) 
	{
		// per-light radiance
		vec3 L = normalize(light_positions[i] - position); // world_pos -> light source
		vec3 H = normalize(V + L);
		float distance = length(light_positions[i] - position);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = light_colors[i] * attenuation;
	
		float n_dot_v = max(dot(N, V), 0.0000001); // prevent divide by 0
		float n_dot_l = max(dot(N, L), 0.0000001); // prevent divide by 0
		float h_dot_v = max(dot(H, V), 0.0);
		float n_dot_h = max(dot(N, H), 0.0);
	
		// Cook-Torrance BRDF
		float NDF = distribution_GGX(n_dot_h, roughness);
		float G   = geometry_smith (n_dot_v, n_dot_l, roughness);
		vec3  F   = fresnel_schlick(h_dot_v, base_reflectivity);
           
		vec3 specular = (NDF * G * F) / (4 * n_dot_v * n_dot_l);
        
		// energy conservation : diffuse + specular < vec3(1) (unless the surface emits light)
		// therefore, the diffuse component = 1.0 - F
		// diffuse * inverse metalness because only non-metals have diffuse lighting
		vec3 diffuse = (vec3(1.0) - F) * (1.0 - metallic);
	
		// note: we multiplied the BRDF by F (in diffuse calculation) so we won't do it again here
		Lo += (diffuse * (albedo / PI) + specular) * radiance * n_dot_l;
	}

	// ambient lighting can be replaced with with environment lighting
	vec3 ambient = vec3(0.03) * albedo * ao;

	vec3 color = ambient + Lo;
	
	color = color / (color + vec3(1.0)); // HDR tonemapping
	color = pow(color, vec3(1.0/2.2)); // gamma correct

	frag_color = vec4(color, 1.0);
}

float distribution_GGX(float n_dot_h, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = n_dot_h * n_dot_h;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return a2 / max(denom, 0.0000001); // prevent divide by zero for roughness=0.0 and NdotH=1.0
}
float geometry_chlick_GGX(float n_dot_v, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

	return n_dot_v / n_dot_v * (1.0 - k) + k;
}
float geometry_smith(float n_dot_v, float n_dot_l, float roughness)
{
    float ggx2 = geometry_chlick_GGX(n_dot_v, roughness);
    float ggx1 = geometry_chlick_GGX(n_dot_l, roughness);

    return ggx1 * ggx2;
}
vec3 fresnel_schlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}