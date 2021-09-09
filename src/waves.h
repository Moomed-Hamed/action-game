#include "intermediary.h"
typedef std::complex<double> Complex;

// fast fourier : result stored in input array
//1D
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

	//if (FFT_BACKWARD == fft_direction) { for (int i = 0; i < size; ++i) { data[i] *= 1.f / size; } }
}
void ifft(Complex* input, uint N)
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

	for (int i = 0; i < N; ++i) { input[i] *= 1.f / N; } // normalize output array
}
//2D
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
void ifft2D(Complex* input, uint N)
{
	Complex* subarray = Alloc(Complex, N); // num_rows = num_columns = N

	for (uint n = 0; n < N; n++) // ifft the columns
	{
		for (int i = 0; i < N; ++i) { subarray[i] = input[(i * N) + n]; }
		ifft(subarray, N);
		for (int i = 0; i < N; ++i) { input[(i * N) + n] = subarray[i]; }
	}

	for (int n = 0; n < N; ++n) // ifft the rows
	{
		for (int i = 0; i < N; ++i) { subarray[i] = input[(n * N) + i]; }
		ifft(subarray, N);
		for (int i = 0; i < N; ++i) { input[(n * N) + i] = subarray[i]; }
	}

	free(subarray);
}
//3D
// ill get to it eventually maybe : just fft2D up the cube, then fft2D across a side

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
		for (uint i = 0; i < N; i++)
		{
			print(" %.2fcos(%dx + %.2f)", abs(output[i]), i, arg(output[i]));
			print("\n");
		}
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
	a[N] = Complex(N * N);
	ifft2D(a, N);
	
	bvec3* img = Alloc(bvec3, N * N); // 3 bytes per channel
	for (uint i = 0; i < N; i++) { // up & down
	for (uint j = 0; j < N; j++)   // left & right
	{
		Complex z = a[(i * N) + j];
		byte r = (abs(z) * cos(arg(z)) * 127) + 128;
		byte g = (abs(z) * cos(arg(z)) * 127) + 128;
		byte b = (abs(z) * cos(arg(z)) * 127) + 128;
		img[(i * N) + j] = { r, g, b };
	} }
	
	stbi_write_bmp("test.bmp", N, N, 3, img);
	free(a);
}

// ocean sim
vec2 gauss_rand() // for generating k (wave vectors)
{
	float x1, x2, w;
	do
	{
		x1 = random_chance_signed();
		x2 = random_chance_signed();
		w = x1 * x1 + x2 * x2;
	} while (w >= 1.f);
	w = sqrt((-2.f * log(w)) / w);
	return vec2(x1 * w, x2 * w);
}

void generate_noise_texture(byte* img, uint N)
{
	for (uint i = 0; i < N; i++) { // up & down
	for (uint j = 0; j < N; j++)   // left & right
	{
		vec2 gr = gauss_rand();
		byte r = (gr.x * 127) + 128;
		byte g = (gr.y * 127) + 128;
		byte b = 0;
		img[(i * N) + j] = { r, g, b };
	} }
	
	stbi_write_bmp("noise.bmp", N, N, 3, img);
}

/*
// initial spectrum
using std::fmin;

vec4* H0;
// wave vector x, 1 / magnitude, wave vector z, frequency
vec4* WavesData;
vec2* H0K;

vec2* Noise; // noise texture
uint Size;
float LengthScale;
float CutoffHigh;
float CutoffLow;
float GravityAcceleration;
float Depth;

struct SpectrumParameters
{
	float scale;
	float angle;
	float spreadBlend;
	float swell;
	float alpha;
	float peakOmega;
	float gamma;
	float shortWavesFade;
} Spectrums[2]; // 0 : local wind, 1 : swell waves

float Frequency(float k, float g, float depth)
{
	return sqrt(g * k * tanh(fmin(k * depth, 20)));
}
float FrequencyDerivative(float k, float g, float depth)
{
	float th = tanh(fmin(k * depth, 20));
	float ch = cosh(k * depth);
	return g * (depth * k / ch / ch + th) / Frequency(k, g, depth) / 2;
}
float NormalisationFactor(float s)
{
	float s2 = s * s;
	float s3 = s2 * s;
	float s4 = s3 * s;
	if (s < 5)
		return -0.000564 * s4 + 0.00776 * s3 - 0.044 * s2 + 0.192 * s + 0.163;
	else
		return -4.80e-08 * s4 + 1.07e-05 * s3 - 9.53e-04 * s2 + 5.90e-02 * s + 3.93e-01;
}
float DonelanBannerBeta(float x)
{
	if (x < 0.95) return 2.61 * pow(abs(x),  1.3);
	if (x < 1.60) return 2.28 * pow(abs(x), -1.3);
	float p = -0.4 + 0.8393 * exp(-0.567 * log(x * x));
	return pow(10, p);
}
float DonelanBanner(float theta, float omega, float peakOmega)
{
	float beta = DonelanBannerBeta(omega / peakOmega);
	float sech = 1 / cosh(beta * theta);
	return beta / 2 / tanh(beta * 3.1416) * sech * sech;
}
float Cosine2s(float theta, float s)
{
	return NormalisationFactor(s) * pow(abs(cos(0.5 * theta)), 2 * s);
}
float SpreadPower(float omega, float peakOmega)
{
	if (omega > peakOmega) {
		return 9.77 * pow(abs(omega / peakOmega), -2.5);
	}
	else {
		return 6.97 * pow(abs(omega / peakOmega), 5);
	}
}
float DirectionSpectrum(float theta, float omega, SpectrumParameters pars)
{
	float s = SpreadPower(omega, pars.peakOmega) + 16 * tanh(fmin(omega / pars.peakOmega, 20)) * pars.swell * pars.swell;
	return lerp(2 / 3.1415 * cos(theta) * cos(theta), Cosine2s(theta - pars.angle, s), pars.spreadBlend);
}
float TMACorrection(float omega, float g, float depth)
{
	float omegaH = omega * sqrt(depth / g);
	if (omegaH <= 1) return 0.5 * omegaH * omegaH;
	if (omegaH <  2) return 1.0 - 0.5 * (2.0 - omegaH) * (2.0 - omegaH);
	return 1;
}
float JONSWAP(float omega, float g, float depth, SpectrumParameters pars)
{
	float sigma;
	if (omega <= pars.peakOmega) sigma = 0.07;
	else sigma = 0.09;

	float r = exp(-(omega - pars.peakOmega) * (omega - pars.peakOmega) / 2 / sigma / sigma / pars.peakOmega / pars.peakOmega);

	float oneOverOmega = 1 / omega;
	float peakOmegaOverOmega = pars.peakOmega / omega;
	return pars.scale * TMACorrection(omega, g, depth) * pars.alpha * g * g
		* oneOverOmega * oneOverOmega * oneOverOmega * oneOverOmega * oneOverOmega
		* exp(-1.25 * peakOmegaOverOmega * peakOmegaOverOmega * peakOmegaOverOmega * peakOmegaOverOmega)
		* pow(abs(pars.gamma), r);
}
float ShortWavesFade(float kLength, SpectrumParameters pars)
{
	return exp(-pars.shortWavesFade * pars.shortWavesFade * kLength * kLength);
}

void CalculateInitialSpectrum(uvec3 id)
{
	float deltaK = 2 * PI / LengthScale;
	int nx = id.x - Size / 2;
	int nz = id.y - Size / 2;
	vec2 k = vec2(nx, nz) * deltaK;
	float kLength = length(k);

	uint index = 0;// something

	if (kLength <= CutoffHigh && kLength >= CutoffLow)
	{
		float kAngle = atan2(k.y, k.x);
		float omega = Frequency(kLength, GravityAcceleration, Depth);
		WavesData[index] = vec4(k.x, 1 / kLength, k.y, omega);
		float dOmegadk = FrequencyDerivative(kLength, GravityAcceleration, Depth);

		float spectrum = JONSWAP(omega, GravityAcceleration, Depth, Spectrums[0]) * DirectionSpectrum(kAngle, omega, Spectrums[0]) * ShortWavesFade(kLength, Spectrums[0]);
		if (Spectrums[1].scale > 0)
		{
			spectrum += JONSWAP(omega, GravityAcceleration, Depth, Spectrums[1]) * DirectionSpectrum(kAngle, omega, Spectrums[1]) * ShortWavesFade(kLength, Spectrums[1]);
		}
		H0K[index] = vec2(Noise[index].x, Noise[index].y) * sqrt(2 * spectrum * abs(dOmegadk) / kLength * deltaK * deltaK);
	}
	else
	{
		H0K[index] = {};
		WavesData[index] = vec4(k.x, 1, k.y, 0);
	}
}
void CalculateConjugatedSpectrum(uvec3 id)
{
	uint index = 0; // WARNING
	vec2 h0K = H0K[index];
	vec2 h0MinusK = H0K[uvec2((Size - id.x) % Size, (Size - id.y) % Size)];
	H0[index] = vec4(h0K.x, h0K.y, h0MinusK.x, -h0MinusK.y);
}
*/