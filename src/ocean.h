#include "height.h"

struct Ocean
{
	int   N; // resolution, must be power of 2
	float L; // side length of ocean patch
	vec2  w; // wind vector
	float A; // affects wave size, fudges philips spectrum

	vec4* d; // xyz = displacement, w = foam
	vec3* n; // normal
	Complex* h0, *h0_conj; // intitial spectrum & its conjugate
	Complex* h, *hmx, *hmz, *hdx, *hdz; // mx = slope_x
	union { GLuint tex[2]; struct { GLuint height, normal; }; };
};

float phillips(vec2 k, vec2 w, float A)
{
	float kl = length(k); if (kl < .000001) { return 0.0; }

	float k2 = kl * kl;
	float k4 = k2 * k2;

	float kw  = dot(normalize(k), normalize(w));
	float kw2 = kw * kw;

	float wl = length(w);
	float l  = wl * wl / 9.81;
	float l2 = l * l;

	float damping = 0.001;
	float dl2 = l2 * damping * damping;

	return A * exp(-1.f / (k2 * l2)) / k4 * kw2 * exp(-k2 * dl2);
}
float dispersion(float k_length, float g = 9.81)
{
	return sqrt(k_length * g);
}
Complex h0(vec2 k, uint N, float L, vec2 w, float A = .01)
{
	return gaussian_random_complex() * sqrt(phillips(k, w, A) / 2.0);
}
Complex ht(Complex h0, Complex h0mkc, uint N, float k_length, float L, float t)
{
	float wt = dispersion(k_length) * t;
	float c = cos(wt), s = sin(wt);
	return h0 * Complex(c, s) + h0mkc * Complex(c, -s);
}

void init(Ocean* ocean, vec2 w, float L = 6, uint N = 256)
{
	*ocean = {};
	ocean->h   = Alloc(Complex, N * N);
	ocean->hmx = Alloc(Complex, N * N);
	ocean->hmz = Alloc(Complex, N * N);
	ocean->hdx = Alloc(Complex, N * N);
	ocean->hdz = Alloc(Complex, N * N);

	ocean->d = Alloc(vec4, N * N);
	ocean->n = Alloc(vec3, N * N);
	ocean->h0      = Alloc(Complex, N * N);
	ocean->h0_conj = Alloc(Complex, N * N);

	ocean->N = N;
	ocean->L = L;
	ocean->w = w;

	float k_min = (-PI * N) / L;
	float dk = TWOPI / L; // delta_k

	for (int x = 0; x < N; x++) { // x
	for (int z = 0; z < N; z++)   // z
	{
		vec2 k = vec2(k_min) + vec2(x * dk, z * dk);
		vec2 k_conj = vec2(k_min) + vec2(-x * dk, -z * dk);

		uint i = x * N + z;
		ocean->h0      [i] = h0(k, N, L, w);
		ocean->h0_conj [i] = std::conj(h0(k_conj, N, L, w));
		ocean->n       [i] = vec3(0, 1, 0);
		ocean->d       [i] = {};
	} }

	glGenTextures(2, ocean->tex); // height + turbulence, normal
	
	glBindTexture(GL_TEXTURE_2D, ocean->height);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, N, N, 0, GL_RGBA, GL_FLOAT, ocean->d);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glBindTexture(GL_TEXTURE_2D, ocean->normal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, N, N, 0, GL_RGB, GL_FLOAT, ocean->n);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
void calculate_waves(Ocean& ocean, float t)
{
	uint N = ocean.N;
	float k_min = -1 * ((PI * N) / ocean.L);
	float dk = TWOPI / ocean.L; // delta_k

	for (int x = 0; x < N; x++) {
	for (int z = 0; z < N; z++)
	{
		vec2 k = vec2(k_min) + vec2(x * dk, z * dk);
		float len = length(k);

		uint i = (x * N) + z;
		ocean.h  [i] = ht(ocean.h0[i], ocean.h0_conj[i], N, len, ocean.L, t);
		ocean.hmx[i] = ocean.h[i] * Complex(0, k.x);
		ocean.hmz[i] = ocean.h[i] * Complex(0, k.y);
		ocean.hdx[i] = len < .0001f ? Complex(0) : ocean.h[i] * Complex(0, -k.x / len);
		ocean.hdz[i] = len < .0001f ? Complex(0) : ocean.h[i] * Complex(0, -k.y / len);
	} }

	ifft2D(ocean.h  , N, false); // after ifft, h.real = y displacement
	ifft2D(ocean.hmx, N, false);
	ifft2D(ocean.hmz, N, false);
	ifft2D(ocean.hdx, N, false);
	ifft2D(ocean.hdz, N, false);

	float lambda = -1.f; // what is this?

	for (int x = 0; x < N; x++) {
	for (int z = 0; z < N; z++)
	{
		uint i  = (x * N) + z;

		float sign = ((x + z) & 1) ? -1 : 1; // i don't know why this is neccessary
		ocean.h  [i] *= sign;
		ocean.hdx[i] *= sign;
		ocean.hdz[i] *= sign;
		ocean.hmx[i] *= sign;
		ocean.hmz[i] *= sign;

		// displacement & normal
		ocean.d[i].y = ocean.h[i].real();
		ocean.d[i].x = ocean.hdx[i].real() * lambda;
		ocean.d[i].z = ocean.hdz[i].real() * lambda;
		ocean.n[i] = normalize(vec3(ocean.hmx[i].real(), -1, ocean.hmz[i].real())) * -1.f;
	} }

	vec4* d = ocean.d;

	for (int x = 0; x < N; x++) {
	for (int z = 0; z < N; z++)
	{
		// index for center finite difference. (left, right, up , down)
		int il = (x - 1 < 0) ? N - 1 : x - 1;
		int ir = (x + 1) % N;
		int jd = (z + 1) % N;
		int ju = (z - 1 < 0) ? N - 1 : z - 1;

		// x, z displacement; lambda has already been multiplied in calculate_waves()
		float dxr = d[ir * N + z ].x;
		float dxl = d[il * N + z ].x;
		float dzu = d[x  * N + ju].z;
		float dzd = d[x  * N + jd].z;

		// Jacobian (or folding map)
		float jxx = 1.f + (dxr - dxl) * 0.5f;
		float jzz = 1.f + (dzu - dzd) * 0.5f;
		float jzx = (dzu - dzd) * 0.5f;
		float jxz = jzx;
		float jacobian = jxx * jzz - jxz * jzx;

		d[x * N + z].w = jacobian < .98 ? 1 : 0;
	} }

	glBindTexture(GL_TEXTURE_2D, ocean.height);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA32F, N, N, 0, GL_RGBA, GL_FLOAT, ocean.d);
	glBindTexture(GL_TEXTURE_2D, ocean.normal);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB32F, N, N, 0, GL_RGB, GL_FLOAT, ocean.n);
}
void recalculate(Ocean* ocean, vec2 w, float A = .01)
{
	uint N = ocean->N;
	float L = ocean->L;

	ocean->w = w;

	float k_min = (-PI * N) / L;
	float dk = TWOPI / L; // delta_k

	for (int x = 0; x < N; x++) { // x
	for (int z = 0; z < N; z++)   // z
	{
		vec2 k = vec2(k_min) + vec2(x * dk, z * dk);
		vec2 k_conj = vec2(k_min) + vec2(-x * dk, -z * dk);

		uint i = x * N + z;
		ocean->h0      [i] = h0(k, N, L, w, A);
		ocean->h0_conj [i] = std::conj(h0(k_conj, N, L, w));
		ocean->n       [i] = vec3(0, 1, 0);
		ocean->d       [i] = {};
	} }
}