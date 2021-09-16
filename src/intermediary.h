// Copyright (c) 2021 Mohamed Hamed
// Intermediary version 4.10.21

#pragma comment(lib, "winmm")
#pragma comment (lib, "Ws2_32.lib") // networking
#pragma comment(lib, "opengl32")
#pragma comment(lib, "external/GLEW/glew32s")
#pragma comment(lib, "external/GLFW/glfw3")
#pragma comment(lib, "external/OpenAL/OpenAL32.lib")

#define _CRT_SECURE_NO_WARNINGS // because printf is "too dangerous"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb_image.h"
#include "external/stb_image_write.h"

#define GLEW_STATIC
#include <external/GLEW\glew.h> // OpenGL functions
#include <external/GLFW\glfw3.h>// window & input

#include "external/OpenAL/al.h" // for audio
#include "external/OpenAL/alc.h"

#include <vector>
#include <complex>

#include <winsock2.h> // for some reason rearranging these
#include <ws2tcpip.h> // includes breaks everything
#include <Windows.h>
#include <fileapi.h>
#include <iostream>
#include <algorithm> // for min/max
using std::min;

#define out(val) std::cout << ' ' << val << '\n'
#define stop std::cin.get()
#define print printf
#define printvec(vec) printf("%f %f %f\n", vec.x, vec.y, vec.z)
#define Alloc(type, count) (type *)calloc(count, sizeof(type))

struct bvec3 { union { struct { byte x, y, z; }; struct { byte r, g, b; }; }; };
#include <proprietary/mathematics.h> // this is pretty much GLM for now
using glm::lookAt;

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
	uint format, size, sample_rate;
	byte* audio_data = NULL;

	FILE* file = fopen(path, "rb"); // rb = read binary
	if (file == NULL) { print("ERROR : %s not found\n"); stop;  return 0; }

	fread(&format     , sizeof(uint), 1, file);
	fread(&sample_rate, sizeof(uint), 1, file);
	fread(&size       , sizeof(uint), 1, file);

	audio_data = Alloc(byte, size);
	fread(audio_data, sizeof(byte), size, file);
	fclose(file);

	ALuint buffer_id = NULL;
	alGenBuffers(1, &buffer_id);
	alBufferData(buffer_id, format, audio_data, size, sample_rate);

	ALuint source_id = NULL;
	alGenSources(1, &source_id);
	alSourcei(source_id, AL_BUFFER, buffer_id);

	free(audio_data);
	return source_id;
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
float lerp_spring(float start, float end, float amount, float stiffness = 8, float period = 4)
{
	float p = sin(amount * period * PI);
	float s = exp(amount * stiffness * -1);
	float spring = p * s;

	float mix = lerp(abs(p) * s, 1.f - (abs(p) * s), amount);
	return lerp(start, end, mix);
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

// multithreading
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

// rng & noise

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
float random_chance_signed() // random float between -1 and 1
{
	return ((random_chance() * 2) - 1);
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

vec3 shake(float trauma) // perlin shake
{
	uint offset = random_uint() % 64;
	float o1 = ((perlin((trauma + offset + 0) * 1000) * 2) - 1) * trauma;
	float o2 = ((perlin((trauma + offset + 1) * 2000) * 2) - 1) * trauma;
	float o3 = ((perlin((trauma + offset + 2) * 3000) * 2) - 1) * trauma;
	return vec3(o1, o2, o3);
}

// tweening
float bezier3(float b, float c, float t) // a = 0 and d = 1
{
	float s = 1.f - t;
	float t2 = t * t;
	float s2 = s * s;
	float t3 = t2 * t;
	return (3.f * b * s2 * t) + (3.f * c * s * t2) + t3;

	// same as this
	//float l1 = lerp(0, b, t);
	//float l2 = lerp(b, c, t);
	//float l3 = lerp(c, 1, t);
	//
	//return lerp(lerp(l1, l12, t), lerp(l2, l3, t), t);
}
float bezier5(float b, float c, float d, float e, float t) // a = 0 and d = 1
{
	// this implementation is not the fastest
	float a1 = lerp(0, b, t);
	float a2 = lerp(b, c, t);
	float a3 = lerp(c, d, t);
	float a4 = lerp(d, e, t);
	float a5 = lerp(e, 1, t);

	float b1 = lerp(a1, a2, t);
	float b2 = lerp(a2, a3, t);
	float b3 = lerp(a3, a4, t);
	float b4 = lerp(a4, a5, t);

	float c1 = lerp(b1, b2, t);
	float c2 = lerp(b2, b3, t);
	float c3 = lerp(b3, b4, t);

	float d1 = lerp(c1, c2, t);
	float d2 = lerp(c2, c3, t);

	return lerp(d1, d2, t);
}
float bezier7(float b, float c, float d, float e, float f, float g, float t) // a = 0 and d = 1
{
	float s = 1.f - t;
	float s2 = t * t;
	float s3 = s2 * t;
	float s4 = s2 * s2;
	float s5 = s3 * s2;
	float s6 = s3 * s3;

	float t2 = t  * t;
	float t3 = t2 * t;
	float t4 = t2 * t2;
	float t5 = t3 * t2;
	float t6 = t3 * t3;
	float t7 = t3 * t4;

	return (7.f * b * s6 * t) + (21.f * c * s5 * t2) + (35.f * d * s4 * t3) * (35.f * e * s3 * t4) + (21.f * f * s2 * t5) + (7.f * g * s * t6) + t7;
}

float bounce(float t, float a = -.45, float b = .25, float c = .55, float d = .75)
{
	return 1 - abs(bezier5(a, b, c, d, 1 - t));
	//return 1 - abs(bezier3(-.45, 0, 1 - t));
}

/*
struct complex
{
	union
	{
		struct { float real, imag; };
		struct { float r, i; };
	};
};

complex conjugate(complex c)
{
	return complex{ c.real, c.imag * -1 };
}

void mul(complex a, complex b)
{

}*/

typedef std::complex<double> Complex;

// fast fourier : result stored in input array
void fft(Complex* input, uint N)
{
	uint target_index = 0;
	uint bit_mask;

	for (uint i = 0; i < N; ++i)
	{
		if (target_index > i) // compute twiddle factors?
		{
			//swap(input[target_index], input[i]);
			Complex temp = input[target_index];
			input[target_index] = input[i];
			input[i] = temp;
		}

		bit_mask = N;
		while (target_index & (bit_mask >>= 1)) // while bit is 1 : bit_mask = bit_mask >> 1
		{
			// Drop bit : ~ = bitwise NOT
			target_index &= ~bit_mask; // target_index = target_index & (~bit_mask)
		}

		target_index |= bit_mask; // target_index = target_index | bit_mask
	}

	for (uint i = 1; i < N; i <<= 1) // cycle for all bit positions of initial signal
	{
		uint  next  = i << 1;
		float angle = -PI / i; // inverse fft uses PI not -PI
		float sine  = sin(.5 * angle); // supplementary sin

		// multiplier for trigonometric recurrence
		Complex mult = Complex(-2.0 * sine * sine, sin(angle));
		Complex factor = 1.0; // start transform factor

		for (uint j = 0; j < i; ++j) { // iterations through groups with different transform factors
		for (uint k = j; k < N; k += next) // iterations through pairs within group
		{
			uint match = k + i;
			Complex product = input[match] * factor;
			input[match] = input[k] - product;
			input[k]    += product;
		} factor = mult * factor + factor; }
	}
}
void ifft(Complex* input, uint N, bool scale = true)
{
	uint target_index = 0;
	uint bit_mask;

	for (uint i = 0; i < N; ++i)
	{
		if (target_index > i) // compute twiddle factors?
		{
			//swap(input[target_index], input[i]);
			Complex temp = input[target_index];
			input[target_index] = input[i];
			input[i] = temp;
		}

		bit_mask = N;
		while (target_index & (bit_mask >>= 1)) // while bit is 1 : bit_mask = bit_mask >> 1
		{
			// Drop bit : ~ = bitwise NOT
			target_index &= ~bit_mask; // target_index = target_index & (~bit_mask)
		}

		target_index |= bit_mask; // target_index = target_index | bit_mask
	}

	for (uint i = 1; i < N; i <<= 1) // cycle for all bit positions of initial signal
	{
		uint  next  = i << 1;
		float angle = PI / i; // inverse fft uses PI not -PI
		float sine  = sin(.5 * angle); // supplementary sin

		// multiplier for trigonometric recurrence
		Complex mult = Complex(-2.0 * sine * sine, sin(angle));
		Complex factor = 1.0; // start transform factor

		for (uint j = 0; j < i; ++j) { // iterations through groups with different transform factors
		for (uint k = j; k < N; k += next) // iterations through pairs within group
		{
			uint match = k + i;
			Complex product = input[match] * factor;
			input[match] = input[k] - product;
			input[k]    += product;
		} factor = mult * factor + factor; }
	}

	if (scale) for (int i = 0; i < N; ++i) { input[i] *= 1.f / N; } // normalize output array
}
void fft2D(Complex* input, uint N)
{
	Complex* subarray = Alloc(Complex, N); // num_rows = num_columns = N

	for (uint n = 0; n < N; n++) // fft the columns
	{
		for (int i = 0; i < N; ++i) { subarray[i] = input[(i * N) + n]; }
		fft(subarray, N);
		for (int i = 0; i < N; ++i) { input[(i * N) + n] = subarray[i]; }
	}

	for (int n = 0; n < N; ++n) // fft the rows
	{
		for (int i = 0; i < N; ++i) { subarray[i] = input[(n * N) + i]; }
		fft(subarray, N);
		for (int i = 0; i < N; ++i) { input[(n * N) + i] = subarray[i]; }
	}

	free(subarray);
}
void ifft2D(Complex* input, uint N, bool scale = false)
{
	Complex* subarray = Alloc(Complex, N); // num_rows = num_columns = N

	for (uint n = 0; n < N; n++) // ifft the columns
	{
		for (int i = 0; i < N; ++i) { subarray[i] = input[(i * N) + n]; }
		ifft(subarray, N, scale);
		for (int i = 0; i < N; ++i) { input[(i * N) + n] = subarray[i]; }
	}

	for (int n = 0; n < N; ++n) // ifft the rows
	{
		for (int i = 0; i < N; ++i) { subarray[i] = input[(n * N) + i]; }
		ifft(subarray, N, scale);
		for (int i = 0; i < N; ++i) { input[(n * N) + i] = subarray[i]; }
	}

	free(subarray);
}

void save_fft2D(Complex* data, uint N, const char* name = "fft2D.bmp")
{
	bvec3* bitmap = (bvec3*)calloc(N * N, 3); // 3 bytes per channel
	for (int i = 0; i < N; i++) { // up & down
	for (int j = 0; j < N; j++)	// left & right
	{
		Complex z = data[(i * N) + j];
		byte r = (cos(arg(z)) * 127) + 128;
		byte g = (cos(arg(z)) * 127) + 128;
		byte b = (cos(arg(z)) * 127) + 128;
		bitmap[(i * N) + j] = { r , g , b };
	} }

	stbi_write_bmp(name, N, N, 3, (byte*)bitmap);
	free(bitmap);
}
void save_ifft2D(Complex* data, uint N, const char* name = "ifft2D.bmp")
{
	bvec3* bitmap = (bvec3*)calloc(N * N, 3); // 3 bytes per channel
	for (int i = 0; i < N; i++) { // up & down
	for (int j = 0; j < N; j++)	// left & right
	{
		Complex z = data[(i * N) + j];

		byte r = (abs(z) * cos(arg(z)) * 127) + 128;
		byte g = (abs(z) * cos(arg(z)) * 127) + 128;
		byte b = (abs(z) * cos(arg(z)) * 127) + 128;
		bitmap[(i * N) + j] = { r , g , b };
	} }

	stbi_write_bmp(name, N, N, 3, (byte*)bitmap);
	free(bitmap);
}

void fft_demo()
{
	{ // forward dft
		Complex input[8];
		input[0] = Complex(1);
		input[1] = Complex(.707106);
		input[2] = Complex(0);
		input[3] = Complex(-.707106);
		input[4] = Complex(-1);
		input[5] = Complex(-.707106);
		input[6] = Complex(0);
		input[7] = Complex(.707106);

		uint N = 8;     // sample count (always power of 2)
		uint H = N / 2; // always an integer because n is power of 2

		fft(input, N);
		Complex* output = input; // since the data has been transformed

		for (uint i = 0; i < N; i++) out(i << ": " << output[i]);

		out("\n after scaling:");
		for (uint i = 0; i < N; i++) output[i] /= H;
		for (uint i = 0; i < N; i++) out(i << ": " << output[i]);

		out("\n magnitudes:");
		for (uint i = 0; i < N; i++) out(i << ": " << abs(output[i]));

		out("\n result:");
		for (uint i = 0; i < N; i++) print(" %.2fcos(%dx + %.2f)\n", abs(output[i]), i, arg(output[i]));
	}
	out(' ');
	{ // inverse dft
		uint N = 4;
		Complex input[4];
		input[0] = Complex(N * 0); // N * (magnitude, phase)
		input[1] = Complex(N * 1); // N * (magnitude, phase)
		input[2] = Complex(N * 0); // N * (magnitude, phase)
		input[3] = Complex(N * 0); // N * (magnitude, phase)
		ifft(input, N);
		for (uint i = 0; i < N; i++) out(i << ": " << input[i]);
	}

	// --------------
	// note : S = sampling frequency = N/L or N/T (subnote : N/T = Hertz)
	// note : frequency resolution = S/N
	// note : nyquist limit = S/2 = Hertz
	// --------------
	// FORWARD DFT:
	// --------------
	// take N samples on some length L (meters) or time period T (in seconds)
	// perform DFT : result is N complex coefficients
	// compute nyquist limit : S/2 & disgard coefficients above the limit
	// divide remaining coefficients by N/2
	// calculate array a : coefficient magnitudes
	// calculate array b : coefficient phase angles
	// result is sum of a[n]cos(x + b[n])
	// --------------
	// INVERSE DFT:
	// --------------
	// make array a : coefficient magnitudes
	// make array b : coefficient phase angles
	// make array of complex numbers a[n] + ib[n]
	// perform IDFT : result is N samples that are S apart?

	// sine wave sampled at 8 points, if you wanna try it
	//input[0] = Complex(0);
	//input[1] = Complex(.707106);
	//input[2] = Complex(1);
	//input[3] = Complex(.707106);
	//input[4] = Complex(0);
	//input[5] = Complex(-.707106);
	//input[6] = Complex(-1);
	//input[7] = Complex(-.707106);
}
void fft2D_demo()
{
	uint N = 256;
	Complex* a = Alloc(Complex, N * N);
	a[4] = Complex(N * N);
	//a[N] = Complex(N * N);
	ifft2D(a, N);
	save_fft2D(a, N, "demo.bmp");
}


void save_vec3(vec3* data, uint w, uint h, const char* name = "vec3.bmp")
{
	bvec3* bitmap = (bvec3*)calloc(w * h, 3); // 3 bytes per channel
	for (int i = 0; i < w; i++) { // up & down
	for (int j = 0; j < h; j++)	// left & right
	{
		vec3 v = data[(i * w) + j];
		byte r = (v.x * 127) + 128;
		byte g = (v.y * 127) + 128;
		byte b = (v.z * 127) + 128;
		bitmap[(i * w) + j] = { r , g , b };
	} }

	stbi_write_bmp(name, w, h, 3, (byte*)bitmap);
	free(bitmap);
}