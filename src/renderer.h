#include "window_and_input.h"

/* -- how 2 draw a mesh --

	Shader shader = {};
	load(&shader, "vertex_shader.vert", "fragment_shader.frag");

	Drawable_Mesh mesh = {};
	load(mesh, "object.mesh");
	
	bind(shader);
	draw(mesh);
*/

struct Shader { GLuint id; };

void load(Shader* shader, const char* vert_path, const char* frag_path)
{
	char* vert_source = (char*)read_text_file_into_memory(vert_path);
	char* frag_source = (char*)read_text_file_into_memory(frag_path);

	GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert_shader, 1, &vert_source, NULL);
	glCompileShader(vert_shader);

	GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag_shader, 1, &frag_source, NULL);
	glCompileShader(frag_shader);

	free(vert_source);
	free(frag_source);

	{
		GLint log_size = 0;
		glGetShaderiv(vert_shader, GL_INFO_LOG_LENGTH, &log_size);
		if (log_size)
		{
			char* error_log = (char*)calloc(log_size, sizeof(char));
			glGetShaderInfoLog(vert_shader, log_size, NULL, error_log);
			out("VERTEX SHADER ERROR:\n" << error_log);
			free(error_log);
		}

		log_size = 0;
		glGetShaderiv(frag_shader, GL_INFO_LOG_LENGTH, &log_size);
		if (log_size)
		{
			char* error_log = (char*)calloc(log_size, sizeof(char));
			glGetShaderInfoLog(frag_shader, log_size, NULL, error_log);
			out("FRAGMENT SHADER ERROR:\n" << error_log);
			free(error_log);
		}
	}

	shader->id = glCreateProgram();
	glAttachShader(shader->id, vert_shader);
	glAttachShader(shader->id, frag_shader);
	glLinkProgram (shader->id);

	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);
}
void bind(Shader shader)
{
	glUseProgram(shader.id);
}
void free(Shader shader)
{
	glDeleteShader(shader.id);
}

// make sure you bind a shader *before* calling these!
void set_int  (Shader shader, const char* name, int value  )
{
	glUniform1i(glGetUniformLocation(shader.id, name), value);
}
void set_float(Shader shader, const char* name, float value)
{
	glUniform1f(glGetUniformLocation(shader.id, name), value);
}
void set_vec3 (Shader shader, const char* name, vec3 value )
{
	glUniform3f(glGetUniformLocation(shader.id, name), value.x, value.y, value.z);
}
void set_mat4 (Shader shader, const char* name, mat4 value )
{
	glUniformMatrix4fv(glGetUniformLocation(shader.id, name), 1, GL_FALSE, (float*)&value);
}

struct Mesh_Data
{
	uint num_vertices, num_indices;

	vec3* positions;
	vec3* normals;
	uint* indices;
};

struct Mesh_Data_UV
{
	uint num_vertices, num_indices;

	vec3* positions;
	vec3* normals;
	vec2* textures;
	uint* indices;
};

void load(Mesh_Data* data, const char* path)
{
	FILE* mesh_file = fopen(path, "rb");
	if (!mesh_file) { print("could not open mesh file : %s\n", path); stop; return; }

	fread(&data->num_vertices, sizeof(uint), 1, mesh_file);
	fread(&data->num_indices , sizeof(uint), 1, mesh_file);

	data->positions = (vec3*)calloc(data->num_vertices, sizeof(vec3));
	data->normals   = (vec3*)calloc(data->num_vertices, sizeof(vec3));
	data->indices   = (uint*)calloc(data->num_indices , sizeof(uint));

	fread(data->positions, sizeof(vec3), data->num_vertices, mesh_file);
	fread(data->normals  , sizeof(vec3), data->num_vertices, mesh_file);
	fread(data->indices  , sizeof(uint), data->num_indices , mesh_file);
}
void load(Mesh_Data_UV* data, const char* path)
{
	FILE* mesh_file = fopen(path, "rb");
	if (!mesh_file) { print("could not open model file: %s\n", path); stop; return; }

	fread(&data->num_vertices, sizeof(uint), 1, mesh_file);
	fread(&data->num_indices , sizeof(uint), 1, mesh_file);

	data->positions = (vec3*)calloc(data->num_vertices, sizeof(vec3));
	data->normals   = (vec3*)calloc(data->num_vertices, sizeof(vec3));
	data->textures  = (vec2*)calloc(data->num_vertices, sizeof(vec2));
	data->indices   = (uint*)calloc(data->num_indices , sizeof(uint));

	fread(data->positions, sizeof(vec3), data->num_vertices, mesh_file);
	fread(data->normals  , sizeof(vec3), data->num_vertices, mesh_file);
	fread(data->textures , sizeof(vec2), data->num_vertices, mesh_file);
	fread(data->indices  , sizeof(uint), data->num_indices , mesh_file);

	fclose(mesh_file);
}

struct Drawable_Mesh
{
	GLuint VAO, VBO, EBO;
	uint num_indices;
};

struct Drawable_Mesh_UV
{
	GLuint VAO, VBO, EBO;
	uint num_indices, texture_id;
};

void load(Drawable_Mesh* mesh, const char* path, uint reserved_mem_size = 0)
{
	Mesh_Data mesh_data;
	load(&mesh_data, path);
	mesh->num_indices = mesh_data.num_indices;

	glGenVertexArrays(1, &(mesh->VAO));
	glBindVertexArray(mesh->VAO);

	uint vertmemsize = mesh_data.num_vertices * sizeof(vec3);
	uint offset = reserved_mem_size;

#define RENDER_MEM_SIZE (reserved_mem_size + (mesh_data.num_vertices * sizeof(vec3) * 2)) // '2' for positions & normals
	glGenBuffers(1, &(mesh->VBO));
	glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);
	glBufferData(GL_ARRAY_BUFFER, RENDER_MEM_SIZE, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, offset, vertmemsize, mesh_data.positions);
	glBufferSubData(GL_ARRAY_BUFFER, offset + vertmemsize, vertmemsize, mesh_data.normals);
#undef RENDER_MEM_SIZE

	glGenBuffers(1, &(mesh->EBO));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh_data.num_indices * sizeof(uint), mesh_data.indices, GL_STATIC_DRAW);

	free(mesh_data.positions);
	free(mesh_data.normals);
	free(mesh_data.indices);

	offset = reserved_mem_size;
	{
		GLint pos_attrib = 0; // local position of a vertex
		glVertexAttribPointer(pos_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)offset);
		glEnableVertexAttribArray(pos_attrib);

		GLint norm_attrib = 1; offset += vertmemsize; // normal of a vertex
		glVertexAttribPointer(norm_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)offset);
		glEnableVertexAttribArray(norm_attrib);
	}
}
void update(Drawable_Mesh mesh, uint vb_size, byte* vb_data)
{
	glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vb_size, vb_data);
}
void draw(Drawable_Mesh mesh, uint num_instances)
{
	glBindVertexArray(mesh.VAO);
	glDrawElementsInstanced(GL_TRIANGLES, mesh.num_indices, GL_UNSIGNED_INT, 0, num_instances);
}

void load(Drawable_Mesh_UV* mesh, const char* path, const char* texture_path, uint reserved_mem_size)
{
	Mesh_Data_UV mesh_data = {};
	load(&mesh_data, path);
	mesh->num_indices = mesh_data.num_indices;

	glGenVertexArrays(1, &(mesh->VAO));
	glBindVertexArray(mesh->VAO);

	uint vertmemsize = mesh_data.num_vertices * sizeof(vec3);
	uint texmemsize  = mesh_data.num_vertices * sizeof(vec2);
	uint offset = reserved_mem_size;

#define RENDER_MEM_SIZE (reserved_mem_size + (vertmemsize * 2) + texmemsize)
	glGenBuffers(1, &(mesh->VBO));
	glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);
	glBufferData(GL_ARRAY_BUFFER, RENDER_MEM_SIZE, NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, offset, vertmemsize, mesh_data.positions);
	glBufferSubData(GL_ARRAY_BUFFER, offset + vertmemsize, vertmemsize, mesh_data.normals);
	glBufferSubData(GL_ARRAY_BUFFER, offset + vertmemsize + vertmemsize, texmemsize, mesh_data.textures);
#undef RENDER_MEM_SIZE

	glGenBuffers(1, &(mesh->EBO));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh_data.num_indices * sizeof(uint), mesh_data.indices, GL_STATIC_DRAW);

	free(mesh_data.positions);
	free(mesh_data.normals);
	free(mesh_data.textures);
	free(mesh_data.indices);

	offset = reserved_mem_size;
	{
		GLint pos_attrib = 0; // position
		glVertexAttribPointer(pos_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)offset);
		glEnableVertexAttribArray(pos_attrib);

		GLint norm_attrib = 1; offset += vertmemsize; // normal
		glVertexAttribPointer(norm_attrib, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)offset);
		glEnableVertexAttribArray(norm_attrib);

		GLint tex_attrib = 2; offset += vertmemsize; // texture coordinates
		glVertexAttribPointer(tex_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void*)offset);
		glEnableVertexAttribArray(tex_attrib);
	}

	if (texture_path)
	{
		int width, height, num_channels;
		byte* image;

		stbi_set_flip_vertically_on_load(true);

		image = stbi_load(texture_path, &width, &height, &num_channels, 0);

		glGenTextures(1, &(mesh->texture_id));
		glBindTexture(GL_TEXTURE_2D, mesh->texture_id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(image);
	}
}
void update(Drawable_Mesh_UV mesh, uint vb_size, byte* vb_data)
{
	glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, vb_size, vb_data);
}
void bind_texture(Drawable_Mesh_UV mesh, uint texture_unit = 0)
{
	glActiveTexture(GL_TEXTURE0 + texture_unit);
	glBindTexture(GL_TEXTURE_2D, mesh.texture_id);
}
void draw(Drawable_Mesh_UV mesh, uint num_instances = 1)
{
	glBindVertexArray(mesh.VAO);
	glDrawElementsInstanced(GL_TRIANGLES, mesh.num_indices, GL_UNSIGNED_INT, 0, num_instances);
}

void mesh_add_attrib_float(GLuint attrib_id, uint stride, uint offset)
{
	glVertexAttribPointer(attrib_id, 1, GL_FLOAT, GL_FALSE, stride, (void*)offset);
	glVertexAttribDivisor(attrib_id, 1);
	glEnableVertexAttribArray(attrib_id);
}
void mesh_add_attrib_vec2 (GLuint attrib_id, uint stride, uint offset)
{
	glVertexAttribPointer(attrib_id, 2, GL_FLOAT, GL_FALSE, stride, (void*)offset);
	glVertexAttribDivisor(attrib_id, 1);
	glEnableVertexAttribArray(attrib_id);
}
void mesh_add_attrib_vec3 (GLuint attrib_id, uint stride, uint offset)
{
	glVertexAttribPointer(attrib_id, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
	glVertexAttribDivisor(attrib_id, 1);
	glEnableVertexAttribArray(attrib_id);
}
void mesh_add_attrib_mat3 (GLuint attrib_id, uint stride, uint offset)
{
	glVertexAttribPointer(attrib_id, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
	glVertexAttribDivisor(attrib_id, 1);
	glEnableVertexAttribArray(attrib_id);

	++attrib_id;
	offset += sizeof(vec3);
	glVertexAttribPointer(attrib_id, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
	glVertexAttribDivisor(attrib_id, 1);
	glEnableVertexAttribArray(attrib_id);

	++attrib_id;
	offset += sizeof(vec3);
	glVertexAttribPointer(attrib_id, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
	glVertexAttribDivisor(attrib_id, 1);
	glEnableVertexAttribArray(attrib_id);
}

/* -- deferred rendering theory --

	glBindFramebuffer(GL_FRAMEBUFFER, g_buffer.FBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	bind(mesh_shader);
	draw(mesh);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	bind(lighting_shader);
	g_buffer_draw(g_buffer);
*/

struct G_Buffer
{
	GLuint FBO; // frame buffer object
	GLuint positions, normals, albedo; // textures
	GLuint VAO, VBO, EBO; // for drawing the quad
};

void init_g_buffer(G_Buffer* buf, Window window)
{
	glGenFramebuffers(1, &buf->FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, buf->FBO);

	GLuint g_positions, g_normals, g_albedo;

	// position color buffer
	glGenTextures(1, &g_positions);
	glBindTexture(GL_TEXTURE_2D, g_positions);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, window.screen_width, window.screen_height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// normal color buffer
	glGenTextures(1, &g_normals);
	glBindTexture(GL_TEXTURE_2D, g_normals);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window.screen_width, window.screen_height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// albedo color buffer
	glGenTextures(1, &g_albedo);
	glBindTexture(GL_TEXTURE_2D, g_albedo);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, window.screen_width, window.screen_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	uint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_positions, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, g_normals  , 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, g_albedo   , 0);
	glDrawBuffers(3, attachments);

	// then also add render buffer object as depth buffer and check for completeness.
	uint depth_render_buffer;
	glGenRenderbuffers(1, &depth_render_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_render_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window.screen_width, window.screen_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_render_buffer);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) { out("FRAMEBUFFER ERROR : INCOMPLETE"); stop; }

	buf->positions = g_positions;
	buf->normals   = g_normals;
	buf->albedo    = g_albedo;

	// make a screen quad

	float verts[] = {
		// X     Y
		-1.f, -1.f, // 0  1-------3
		-1.f,  1.f, // 1  |       |
		 1.f, -1.f, // 2  |       |
		 1.f,  1.f  // 3  0-------2
	};

	float tex_coords[]{
		// X     Y
		0.f, 0.f, // 0  1-------3
		0.f, 1.f, // 1  |       |
		1.f, 0.f, // 2  |       |
		1.f, 1.f  // 3  0-------2
	};

	uint indicies[] = {
		0,2,3,
		3,1,0
	};

	glGenVertexArrays(1, &buf->VAO);
	glBindVertexArray(buf->VAO);

	glGenBuffers(1, &buf->VBO);
	glBindBuffer(GL_ARRAY_BUFFER, buf->VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts) + sizeof(tex_coords), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(verts), sizeof(tex_coords), tex_coords);

	glGenBuffers(1, &buf->EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf->EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicies), indicies, GL_STATIC_DRAW);

	uint offset = 0;
	GLint vert_attrib = 0; // vertex position
	glVertexAttribPointer(vert_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void*)offset);
	glEnableVertexAttribArray(vert_attrib);

	offset += sizeof(verts);
	GLint tex_attrib = 1; // texture coordinates
	glVertexAttribPointer(tex_attrib, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void*)offset);
	glEnableVertexAttribArray(tex_attrib);
}
void draw_g_buffer(G_Buffer g_buffer)
{
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, g_buffer.positions);
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, g_buffer.normals);
	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, g_buffer.albedo);

	glBindVertexArray(g_buffer.VAO);
	glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, 1);
}

/* -- deferred rendering cheat sheet --

	Position texture = 0
	Normal   texture = 1
	Albedo   texture = 2

	GL_FRAMEBUFFER = 0 for default framebuffer
*/

struct Point_Light
{
	vec3 position;
	vec3 color;
	float linear, quadratic;
};

struct Spot_Light
{
	vec3 position;
	vec3 direction;
	float inner_cuttof, outer_cuttof;
	vec3 color;
	float linear, quadratic;
};

struct Directional_Light
{
	vec3 direction;
	vec3 color;
	float linear, quadratic;
};

struct Point_Light_Drawable
{
	Shader shader;
	Drawable_Mesh mesh;
};

struct Spot_Light_Drawable
{
	Shader shader;
	Drawable_Mesh mesh;
};

struct Directional_Light_Drawable
{
	Shader shader;
	Drawable_Mesh mesh;
};

struct Light_Renderer
{
	uint num_point_lights, num_spot_lights, num_directional_lights;
	Point_Light_Drawable* point_lights;
	Spot_Light_Drawable* spot_lights;
	Directional_Light_Drawable* directional_lights;
};

void init(Light_Renderer* renderer)
{

}

Shader make_lighting_shader()
{
	Shader lighting_shader = {};
	load(&lighting_shader, "assets/shaders/lighting.vert", "assets/shaders/lighting.frag");
	bind(lighting_shader);

	set_int(lighting_shader, "positions", 0);
	set_int(lighting_shader, "normals"  , 1);
	set_int(lighting_shader, "albedo"   , 2);
	
	// Directional Lighting
	vec3 light_direction = vec3(-1, -1, -1);
	set_vec3(lighting_shader, "dir_light.direction", light_direction);
	set_vec3(lighting_shader, "dir_light.direction", light_direction);
	set_vec3(lighting_shader, "dir_light.ambient"  , vec3(0.2f, 0.2f, 0.2f));
	set_vec3(lighting_shader, "dir_light.diffuse"  , vec3(0.5f, 0.5f, 0.5f));
	set_vec3(lighting_shader, "dir_light.specular" , vec3(1.0f, 1.0f, 1.0f));
	
	// Point Lights
	vec3 light_position = vec3(8, 65, 8);
	set_vec3 (lighting_shader, "pt_light.position" , light_position);
	set_vec3 (lighting_shader, "pt_light.ambient"  , vec3(0.2f, 0.2f, 0.2f));
	set_vec3 (lighting_shader, "pt_light.diffuse"  , vec3(0.5f, 0.5f, 0.5f));
	set_vec3 (lighting_shader, "pt_light.specular" , vec3(1.0f, 1.0f, 1.0f));
	set_float(lighting_shader, "pt_light.linear"   , 0.09f);
	set_float(lighting_shader, "pt_light.quadratic", 0.032f);
	
	// Spot Lights
	float spt_light_cutoff_inner = cos(ToRadians(12.5f));
	float spt_light_cutoff_outer = cos(ToRadians(17.5f));
	set_vec3 (lighting_shader, "spt_light.position"    , vec3(2, 1.5, 2));
	set_vec3 (lighting_shader, "spt_light.direction"   , vec3(2, 1.5, 2));
	set_float(lighting_shader, "spt_light.inner_cutoff", spt_light_cutoff_inner);
	set_float(lighting_shader, "spt_light.outer_cutoff", spt_light_cutoff_outer);
	set_vec3 (lighting_shader, "spt_light.ambient"     , vec3(.001, 0, 0));
	set_vec3 (lighting_shader, "spt_light.diffuse"     , vec3(1, 0, 0));
	set_vec3 (lighting_shader, "spt_light.specular"    , vec3(1, 0, 0));
	set_float(lighting_shader, "spt_light.linear"      , 0.01f);
	set_float(lighting_shader, "spt_light.quadratic"   , 0.032f);

	return lighting_shader;
}