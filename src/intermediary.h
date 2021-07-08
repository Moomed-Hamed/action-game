// Copyright (c) 2021 Mohamed Hamed
// Intermediary version 7.7.21

#pragma comment(lib, "winmm")
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