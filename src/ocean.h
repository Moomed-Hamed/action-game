#include "height.h"

Complex gaussian_random_complex()
{
	float x1, x2, w;
	do {
		x1 = random_chance_signed();
		x2 = random_chance_signed();
		w = x1 * x1 + x2 * x2;
	} while (w > 1.f);
	w = sqrt((-2.f * log(w)) / w);
	return Complex(x1 * w, x2 * w);
}
float peak_speed(float windspeed, float fetch = 100000, float g = 9.81)
{
	return 22 * pow((g * g) / (windspeed * fetch), .333333f); // peak wave_speed
}
float wave_speed(float k, float g = 9.81)
{
	return sqrt(g * k); // wave_speed
}
float wave_speed_derivative(float k_length, float depth = 500, float g = 9.81)
{
	float k = k_length; // makes it look nicer; sue me
	float th = (k * depth < 20) ? k * depth : 20;
	float ch = cosh(k * depth);
	return g * (depth * k / ch / ch + th) / wave_speed(k) / 2;
}
float directional_spread(float theta, float speed, float max_speed, float swell, float spread_blend = 1)
{
	// Hasselmann directional spread
	float m = abs(speed / max_speed); // m for magnitude, duh
	float s = speed > max_speed ? 9.77 * pow(m, -2.5) : 6.97 * pow(m, 5);

	float wind_angle = swell > .5 ? ToRadians(0) : ToRadians(-29.81); // idk if/why this should exist

	//cosine2s
	float s2 = s * s;
	float s3 = s2 * s;
	float s4 = s3 * s;
	float normalization_factor = 0;
	if (s < 5)
		normalization_factor = -0.000564 * s4 + 0.00776 * s3 - 0.044 * s2 + 0.192 * s + 0.163;
	else
		normalization_factor = -4.80e-08 * s4 + 1.07e-05 * s3 - 9.53e-04 * s2 + 5.90e-02 * s + 3.93e-01;

	float ct = cos(theta);
	float cosine2s = normalization_factor * pow(abs(cos((theta - wind_angle) * 0.5)), 2 * s);

	s += 16 * tanh(min(speed / max_speed, 20.f)) * swell * swell;
	return lerp(2 / PI * ct * ct, cosine2s, spread_blend);
}
float jonswap(float speed, float max_speed, float windspeed, float fetch = 800000, float g = 9.81)
{
	float gamma = 3.3; // jonswap gamma (maybe make this a variable later)
	float g2 = g * g;

	float c = speed;
	float a = .076 * pow(windspeed / (fetch * g), .22f); // jonswap alpha
	float m = max_speed;
	float s = c < m ? .07 : .09; // sigma (grindset)

	float op = c - m;
	float sp = s * m; // (a * a) * (b * b) = (a * b) * (a * b)
	float r = exp(-1 * op * op / (2 * sp * sp));

	float m4 = m * m * m * m;
	float c4 = c * c * c * c;
	float c5 = c4 * c;

	// TMA correction
	float TMA = 1;
	float depth = 500; // water depth
	float c_h = c * sqrt(depth / g);
	if (c_h < 1) c_h = 0.5 * c_h * c_h;
	else if (c_h < 2) c_h = 1.0 - 0.5 * (2.0 - c_h) * (2.0 - c_h);

	return TMA * (a * g2 / c5) * exp((-5 / 4) * (m4 / c4)) * pow(gamma, r);
}
float short_waves_fade(float k_length, float shortwavesfade = 0.01)
{
	return exp(-shortwavesfade * shortwavesfade * k_length * k_length);
}

void make_stationary_spectrum(vec2 wind, float l = 250, uint N = 256)
{
	Complex* h = Alloc(Complex, N * N);

	float wind_speed = length(wind);
	float c_max = peak_speed(wind_speed); // max wave_speed for this wind_speed

	float k_min = (-PI * N) / l;
	float k_max = (sqrt(2) * TWOPI) / l;
	float dk = TWOPI / l; // delta_k

	for (int x = 0; x < N; x++) {
	for (int z = 0; z < N; z++)
	{
		vec2 k = vec2(k_min) + vec2(x * dk, z * dk); // wave vector
		float k_length = length(k); // wave number
		float k_angle = PI * dot(normalize(k), normalize(wind));
		float c = wave_speed(k_length); // wave_speed

		float spectrum = jonswap(c, c_max, wind_speed) * directional_spread(k_angle, c, c_max, .198) * short_waves_fade(k_length);
		float dwdk = abs(wave_speed_derivative(k_length));
		float ki = 1 / k_length; // k_inverse
		h[(x * N) + z] = gaussian_random_complex() * sqrt(2.0 * spectrum * dwdk * ki * dk * dk);
	} }

	// save as image
	bvec3* bitmap = (bvec3*)calloc(N * N, 3);
	for (int i = 0; i < N; i++) {
	for (int j = 0; j < N; j++)
	{
		Complex z = h[(i * N) + j]; // out(z);

		byte r = (z.real() / 1) * 255;
		byte g = (z.imag() / 1) * 255;

		bitmap[(i * N) + j] = { r , g , 0 };
	} }

	stbi_write_bmp("stationary_spectrum.bmp", N, N, 3, (byte*)bitmap);

	// wave data
	for (int i = 0; i < N; i++) {
	for (int j = 0; j < N; j++)
	{
		vec2 k = vec2(k_min) + vec2(i * dk, j * dk);
		//k = k_max / k;

		byte r = k.x * 255;
		byte g = k.y * 255;
		byte b = 1.f / length(k);

		bitmap[(i * N) + j] = { r , g , b };
	} }

	stbi_write_bmp("wave_data.bmp", N, N, 3, (byte*)bitmap);
}

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
	float k_length = length(k);
	float k_angle = atan2(k.y, k.x);
	float c = wave_speed(k_length); // wave_speed/temporal_frequency
	
	float wind_speed = 10;
	float c_max = peak_speed(wind_speed);
	
	float spectrum = jonswap(c, c_max, wind_speed) * directional_spread(k_angle, c, c_max, .198) * short_waves_fade(k_length);
	float dwdk = abs(wave_speed_derivative(k_length));
	float ki = 1 / k_length; // ki = k_inverse

	float dk = TWOPI / L;
	//return gaussian_rand_complex() * sqrt(2.0 * spectrum * dwdk * ki * dk * dk);
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