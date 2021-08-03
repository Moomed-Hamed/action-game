// Copyright (c) 2021 Mohamed Hamed
// Intermediary version 31.7.21

#pragma comment(lib, "winmm")
#pragma comment (lib, "Ws2_32.lib") // networking
#pragma comment(lib, "opengl32")
#pragma comment(lib, "external/GLEW/glew32s")
#pragma comment(lib, "external/GLFW/glfw3")
#pragma comment(lib, "external/OpenAL/OpenAL32.lib")

#define _CRT_SECURE_NO_WARNINGS // because printf is "too dangerous"

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"

#define GLEW_STATIC
#include <external/GLEW\glew.h> // OpenGL functions
#include <external/GLFW\glfw3.h>// window & input

#include "external/OpenAL/al.h" // for audio
#include "external/OpenAL/alc.h"

#include <winsock2.h> // for some reason rearranging these
#include <ws2tcpip.h> // includes breaks everything
#include <Windows.h>
#include <fileapi.h>
#include <iostream>

#define out(val) std::cout << ' ' << val << '\n'
#define stop std::cin.get()
#define print printf
#define printvec(vec) printf("%f %f %f\n", vec.x, vec.y, vec.z)
#define Alloc(type, count) (type *)calloc(count, sizeof(type))

#include <proprietary/mathematics.h> // this is pretty much GLM for now

byte* read_text_file_into_memory(const char* path)
{
	DWORD BytesRead;
	HANDLE os_file = CreateFile(path, GENERIC_READ | GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	LARGE_INTEGER size;
	GetFileSizeEx(os_file, &size);

	byte* memory = (byte*)calloc(size.QuadPart + 1, sizeof(byte));
	ReadFile(os_file, memory, size.QuadPart, &BytesRead, NULL);

	CloseHandle(os_file);

	return memory;
}
// -- Timers -- // // might be broken idk

// while the raw timestamp can be used for relative performence measurements,
// it does not necessarily correspond to any external notion of time
typedef uint64 Timestamp;

Timestamp get_timestamp()
{
	LARGE_INTEGER win32_timestamp;
	QueryPerformanceCounter(&win32_timestamp);

	return win32_timestamp.QuadPart;
}

int64 calculate_milliseconds_elapsed(Timestamp start, Timestamp end)
{
	//Get CPU clock frequency for Timing
	LARGE_INTEGER win32_performance_frequency;
	QueryPerformanceFrequency(&win32_performance_frequency);

	return (1000 * (end - start)) / win32_performance_frequency.QuadPart;
}
int64 calculate_microseconds_elapsed(Timestamp start, Timestamp end)
{
	//Get CPU clock frequency for Timing
	LARGE_INTEGER win32_performance_frequency;
	QueryPerformanceFrequency(&win32_performance_frequency);

	// i think (end - start) corresponds directly to cpu clock cycles but i'm not sure
	return (1000000 * end - start) / win32_performance_frequency.QuadPart;
}

void os_sleep(uint milliseconds)
{
#define DESIRED_SCHEDULER_GRANULARITY 1 // milliseconds
	HRESULT SchedulerResult = timeBeginPeriod(DESIRED_SCHEDULER_GRANULARITY);
#undef DESIRED_SCHEDULER_GRANULARITY

	Sleep(milliseconds);
}

// --- Audio --- //

/* -- how 2 play a sound --

	Audio sound = load_audio("sound.audio");
	play_sudio(sound);
*/

#define MAX_SOUND_SIZE 2048 // ?

struct Sound
{
	uint size;
	byte data[MAX_SOUND_SIZE];
};

typedef ALuint Audio;

Audio load_audio(const char* path)
{
	uint format;
	uint sample_rate;
	uint size;
	char* audio_data;

	FILE* file = fopen(path, "rb");
	fread(&format, sizeof(uint), 1, file);
	fread(&sample_rate, sizeof(uint), 1, file);
	fread(&size, sizeof(uint), 1, file);
	audio_data = Alloc(char, size);
	fread(audio_data, sizeof(char), size, file);
	fclose(file);

	ALuint bufferid = 0;
	alGenBuffers(1, &bufferid);
	alBufferData(bufferid, format, audio_data, size, sample_rate);

	ALuint SourceID;
	alGenSources(1, &SourceID);
	alSourcei(SourceID, AL_BUFFER, bufferid);

	free(audio_data);
	return SourceID;
}

void play_audio(Audio source_id)
{
	alSourcePlay(source_id);
}

// linear interpolation
float lerp(float start, float end, float amount)
{
	return (start + amount * (end - start));
}
float lerp_sin(float start, float end, float amount)
{
	return lerp(start, end, sin(amount * (PI / 2.f)));
}
vec3 lerp(vec3 start, vec3 end, float amount)
{
	return (start + amount * (end - start));
}
quat lerp(quat start, quat end, float amount)
{
	return (start + amount * (end - start));
}
mat4 lerp(mat4 frame_1, mat4 frame_2, float amount)
{
	vec3 pos_1 = vec3(frame_1[0][3], frame_1[1][3], frame_1[2][3]);
	vec3 pos_2 = vec3(frame_2[0][3], frame_2[1][3], frame_2[2][3]);

	quat rot_1 = quat(frame_1);
	quat rot_2 = quat(frame_2);

	vec3 pos = lerp(pos_1, pos_2, amount);
	quat rot = lerp(rot_1, rot_2, amount);

	mat4 ret = mat4(rot);
	ret[0][3] = pos.x;
	ret[1][3] = pos.y;
	ret[2][3] = pos.z;

	return ret;
}
mat4 lerp_sin(mat4 frame_1, mat4 frame_2, float amount)
{
	vec3 pos_1 = vec3(frame_1[0][3], frame_1[1][3], frame_1[2][3]);
	vec3 pos_2 = vec3(frame_2[0][3], frame_2[1][3], frame_2[2][3]);

	quat rot_1 = quat(frame_1);
	quat rot_2 = quat(frame_2);

	vec3 pos = lerp(pos_1, pos_2, sin(amount * (PI / 2.f)));
	quat rot = lerp(rot_1, rot_2, sin(amount * (PI / 2.f)));

	mat4 ret = mat4(rot);
	ret[0][3] = pos.x;
	ret[1][3] = pos.y;
	ret[2][3] = pos.z;

	return ret;
}
mat4 lerp_spring(mat4 frame_1, mat4 frame_2, float amount)
{
	vec3 pos_1 = vec3(frame_1[0][3], frame_1[1][3], frame_1[2][3]);
	vec3 pos_2 = vec3(frame_2[0][3], frame_2[1][3], frame_2[2][3]);

	quat rot_1 = quat(frame_1);
	quat rot_2 = quat(frame_2);

	float period    = sin(amount * 8.f * PI);
	float stiffness = exp(amount * -5);
	float spring = period * stiffness;

	vec3 pos = lerp(pos_1, pos_2, spring);
	quat rot = lerp(rot_1, rot_2, spring);

	mat4 ret = mat4(rot);
	ret[0][3] = pos.x;
	ret[1][3] = pos.y;
	ret[2][3] = pos.z;

	return ret;
}
mat4 nlerp(mat4 frame_1, mat4 frame_2, float amount)
{
	vec3 pos_1 = vec3(frame_1[0][3], frame_1[1][3], frame_1[2][3]);
	vec3 pos_2 = vec3(frame_2[0][3], frame_2[1][3], frame_2[2][3]);

	quat rot_1 = quat(frame_1);
	quat rot_2 = quat(frame_2);

	vec3 pos = lerp(pos_1, pos_2, amount);
	quat rot = glm::normalize(lerp(rot_1, rot_2, amount));

	mat4 ret = mat4(rot);
	ret[0][3] = pos.x;
	ret[1][3] = pos.y;
	ret[2][3] = pos.z;

	return ret;
}

// threads

typedef DWORD WINAPI thread_function(LPVOID); // what is this sorcery?

DWORD WINAPI thread_func(LPVOID param)
{
	printf("Thread Started");
	return 0;
}

uint64 create_thread(thread_function function, void* params = NULL)
{
	DWORD thread_id = 0;
	CreateThread(0, 0, function, params, 0, &thread_id);
	
	// CreateThread returns a handle to the thread.
	// im not sure if i should be keeping it
	return thread_id;
}

//uint test(float a) { out(a); return 0; }
//typedef uint temp(float);
//void wtf(temp func) { func(7); return; }

// random

#define BIT_NOISE_1 0xB5297A4D;
#define BIT_NOISE_2 0x68E31DA4;
#define BIT_NOISE_3 0x1B56C4E9;

uint random_uint()
{
	uint seed = __rdtsc();
	seed *= BIT_NOISE_1;
	seed *= seed; // helps avoid linearity
	seed ^= (seed >> 8);
	seed += BIT_NOISE_2;
	seed ^= (seed >> 8);
	seed *= BIT_NOISE_3;
	seed ^= (seed >> 8);
	return seed;
}
int random_int()
{
	union
	{
		uint seed;
		int ret;
	} u;

	u.seed = random_uint();

	return u.ret;
}
int random_float()
{
	union {
		uint seed;
		float ret;
	} u;

	u.seed = random_uint();

	return u.ret;
}
float random_chance() // random float between 0 and 1
{
	uint seed = random_uint();
	return (float)seed / (float)UINT_MAX; // is there a better way to do this?
}
bool random_boolean(float probability_of_returning_true = 0.5)
{
	if (random_chance() < probability_of_returning_true) return true;
	
	return false;
}
int noise(uint n, uint seed = 0)
{
	n *= BIT_NOISE_1;
	n += seed;
	n ^= (n >> 8);
	n += BIT_NOISE_2;
	n ^= (n >> 8);
	n *= BIT_NOISE_3;
	n ^= (n >> 8);
	return n;
}
float noise_chance(uint n, uint seed = 0)
{
	n *= BIT_NOISE_1;
	n += seed;
	n ^= (n >> 8);
	n += BIT_NOISE_2;
	n ^= (n >> 8);
	n *= BIT_NOISE_3;
	n ^= (n >> 8);

	return (float)n / (float)UINT_MAX;
}
float perlin(float n)
{
	int x1 = (int)n;
	int x2 = x1 + 1;

	//int gradient_1 = noise(x1);
	//int gradient_2 = noise(x2);

	return lerp(noise_chance(x1), noise_chance(x2), n - (float)x1);
}