#include "bullets.h"

#define ACTION_LOAD	1
#define ACTION_ARM	2
#define ACTION_FIRE	3
#define ACTION_INSPECT	4

#define ACTION_LOAD_TIME	.2f
#define ACTION_ARM_TIME	.25f
#define ACTION_FIRE_TIME	.2f
#define ACTION_INSPECT_TIME	3.f

struct Flintlock
{
	uint action;
	float action_time;

	bool loaded, armed;
	union { Audio sounds[3]; struct { Audio arm, shoot, reload; }; };
};

void update(Flintlock* gun, Bullet* bullets, Camera* cam, Mouse mouse, Keyboard keys, float dtime)
{
	if (gun->action_time < 0)
	{
		switch (gun->action)
		{
		case ACTION_LOAD: gun->loaded = true; break;
		case ACTION_ARM : gun->armed  = true; break;
		case ACTION_FIRE: gun->loaded = gun->armed = false; break;
		}
		gun->action = NULL; // idle
		gun->action_time = -1;
	} else { gun->action_time -= dtime; return; }

	if (mouse.left_button.is_pressed && !mouse.left_button.was_pressed) // attempt to fire
	{
		if (gun->armed)  { goto fire; } // armed & loaded
		if (gun->loaded) { goto arm;  } // loaded & not armed
		goto load; // not loaded & not armed
	}

	if (keys.R.is_pressed && !keys.R.was_pressed)
	{
		if (gun->loaded && gun->armed) return;
		if (gun->loaded && !gun->armed) goto arm;
		if (!gun->loaded && !gun->armed) goto load;
	}

	if (keys.N.is_pressed && !keys.N.was_pressed) { goto inspect; }

	return;

	load: // put a bullet in the gun
	{
		gun->action = ACTION_LOAD;
		gun->action_time = ACTION_LOAD_TIME;
		return;
	}

	arm: // cock the gun
	{
		gun->action = ACTION_ARM;
		gun->action_time = ACTION_ARM_TIME;
		play_audio(gun->arm);
		return;
	}

	fire: // fire the flintlock
	{
		gun->action = ACTION_FIRE;
		gun->action_time = ACTION_FIRE_TIME;
		cam->trauma = .25;
	
		gun->loaded = gun->armed = false;
	
		spawn(bullets, cam->position, cam->front * 10.f);
		play_audio(gun->shoot);
		return;
	}

	inspect: // look at the pretty gun model
	{
		gun->action = ACTION_INSPECT;
		gun->action_time = ACTION_INSPECT_TIME;
		return;
	}
}

// rendering

void update_flintlock_anim(Flintlock gun, Animation* anim, mat4* current_pose, float dtime);

struct Flintlock_Renderer
{
	Drawable_Mesh_Anim_UV mesh;
	Shader shader;
	Animation animation;
	GLuint texture, material;
	mat4 current_pose[MAX_ANIM_BONES];
};

void init(Flintlock_Renderer* renderer)
{
	load(&renderer->mesh, "assets/meshes/weps/flintlock.mesh_anim", sizeof(Prop_Drawable));
	mesh_add_attrib_vec3(5, sizeof(Prop_Drawable), 0); // world pos
	mesh_add_attrib_mat3(6, sizeof(Prop_Drawable), sizeof(vec3)); // rotation

	renderer->texture  = load_texture("assets/textures/palette2.bmp");
	renderer->material = load_texture("assets/textures/materials.bmp");
	load(&(renderer->shader), "assets/shaders/transform/mesh_anim_uv.vert", "assets/shaders/mesh_uv.frag");

	load(&renderer->animation, "assets/animations/flintlock.anim"); // animaiton keyframes
}
void update_renderer(Flintlock_Renderer* renderer, Flintlock gun, float dtime, Camera cam, float turn)
{
	vec3 front = cam.front;
	vec3 right = cam.right;
	vec3 up    = cam.up;

	update_flintlock_anim(gun, &renderer->animation, renderer->current_pose, dtime);

	static float turn_amount = 0; turn_amount += turn;
	if (turn_amount >  .1) turn_amount =  .1;
	if (turn_amount < -.1) turn_amount = -.1;
	vec3 look = lerp(front * -1.f, right, -1 * (-.05 + turn_amount));
	turn_amount *= dtime;

	Prop_Drawable drawable = {};
	drawable.position  = cam.position + (front * .6f) + (up * -.25f) + (right * .3f);
	drawable.transform = mat3(1) * point_at(look, up);

	static float time = 0; time += dtime;
	if (gun.action == NULL)
	{
		drawable.position += up    * (.01f  * sin(time * 2.6f));
		drawable.position += right * (.005f * sin(time * 1.6f));
	} else time = 0;

	update(renderer->mesh, renderer->animation.num_bones, renderer->current_pose, sizeof(Prop_Drawable), (byte*)(&drawable));
}
void draw(Flintlock_Renderer renderer, mat4 proj_view)
{
	bind(renderer.shader);
	bind_texture(renderer.texture , 0);
	bind_texture(renderer.material, 1);
	set_mat4(renderer.shader, "proj_view", proj_view);
	draw(renderer.mesh);
}

// weapon animations
void update_flintlock_anim(Flintlock gun, Animation* anim, mat4* current_pose, float dtime)
{
	switch (gun.action)
	{
	case ACTION_LOAD:
	{
		float completeness = (ACTION_LOAD_TIME - gun.action_time) / ACTION_LOAD_TIME;

		if (completeness < .5) // load
		{
			float mix = completeness * 2;
			update_animation_pose(anim, current_pose, 0, 2, mix);
		}
		else // rest pose
		{
			float mix = (completeness - .5) * 2;
			update_animation_pose(anim, current_pose, 2, 0, mix);
		}
	} break;

	case ACTION_ARM:
	{
		float completeness = (ACTION_ARM_TIME - gun.action_time) / ACTION_ARM_TIME;
		update_animation_pose(anim, current_pose, 0, 1, lerp_sin(0, 1, completeness));
	} break;

	case ACTION_FIRE:
	{
		float completeness = (ACTION_FIRE_TIME - gun.action_time) / ACTION_FIRE_TIME;

		if (completeness < .5) // fire
		{
			float mix = completeness * 2;
			update_animation_pose(anim, current_pose, 0, 2, mix);
		}
		else // rest pose
		{
			float mix = (completeness - .5) * 2;
			update_animation_pose(anim, current_pose, 2, 0, mix);
		}
	} break;

	default: // ACTION_IDLE
	{
		float mix = gun.armed ? 1 : 0;
		update_animation_pose(anim, current_pose, 0, 1, mix);
	} break;
	}
}

// swords

struct Sword
{
	uint type, action;
	float action_timer;
	float damage;
};

// crosshair rendering

struct Quad_Drawable
{
	vec2 position;
	vec2 scale;
	vec3 color;
};

struct Crosshair_Renderer
{
	Quad_Drawable quads[4];
	Drawable_Mesh_2D mesh;
	Shader shader;
};

void init(Crosshair_Renderer* renderer)
{
	*renderer = {};
	init(&renderer->mesh, 4 * sizeof(Quad_Drawable));
	mesh_add_attrib_vec2(1, sizeof(Quad_Drawable), 0); // position
	mesh_add_attrib_vec2(2, sizeof(Quad_Drawable), sizeof(vec2)); // scale
	mesh_add_attrib_vec3(3, sizeof(Quad_Drawable), sizeof(vec2) + sizeof(vec2)); // color

	load(&renderer->shader, "assets/shaders/mesh_2D.vert", "assets/shaders/mesh_2D.frag");

	vec2 position = vec2(0, 0);
	vec2 scale    = vec2(.003, .005) / 2.f;
	vec3 color    = vec3(1, 0, 0);

	renderer->quads[0].position = position + vec2(.006f, 0);
	renderer->quads[0].scale    = scale;
	renderer->quads[0].color    = color;
	renderer->quads[1].position = position - vec2(.006f, 0);
	renderer->quads[1].scale    = scale;
	renderer->quads[1].color    = color;
	renderer->quads[2].position = position + vec2(0, .01f);
	renderer->quads[2].scale    = scale;
	renderer->quads[2].color    = color;
	renderer->quads[3].position = position - vec2(0, .01f);
	renderer->quads[3].scale    = scale;
	renderer->quads[3].color    = color;
}
void update(Crosshair_Renderer* renderer)
{
	update(renderer->mesh, 4 * sizeof(Quad_Drawable), (byte*)renderer->quads);
}
void draw(Crosshair_Renderer* renderer)
{
	bind(renderer->shader);
	draw(renderer->mesh, 4);
}