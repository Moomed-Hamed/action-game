// --  fast fourier -- //

struct FFT
{
	uint N, which;
	uint log_2_N;
	float pi2;
	uint *reversed;
	Complex **W;
	Complex *c[2];
};

uint reverse(uint i, uint log_2_N)
{
	uint res = 0;

	for (uint j = 0; j < log_2_N; j++)
	{
		res = (res << 1) + (i & 1);
		i >>= 1;
	}

	return res;
}
Complex w(uint x, uint N)
{
	return Complex(cos(TWOPI * x / N), sin(TWOPI * x / N));
}

void init(FFT* fft, uint N)
{
	fft->N = N;
	fft->log_2_N = log(N) / log(2);
	fft->c[0] = new Complex[N];
	fft->c[1] = new Complex[N];
	fft->which = 0;

	fft->reversed = new uint[N]; // prep bit reversals
	for (int i = 0; i < N; i++) fft->reversed[i] = reverse(i, fft->log_2_N);

	int pow2 = 1;
	fft->W = new Complex*[fft->log_2_N]; // prep W
	for (int i = 0; i < fft->log_2_N; i++)
	{
		fft->W[i] = new Complex[pow2];
		for (int j = 0; j < pow2; j++) { fft->W[i][j] = w(j, pow2 * 2); }
		pow2 *= 2;
	}
}
void fast_fourier_transform(FFT* fft, Complex *input, Complex *output, int stride, int offset) {
	for (int i = 0; i < fft->N; i++)
		fft->c[fft->which][i] = input[fft->reversed[i] * stride + offset];

	int loops = fft->N >> 1;
	int size = 1 << 1;
	int size_over_2 = 1;
	int w_ = 0;

	for (int i = 1; i <= fft->log_2_N; i++) {
		fft->which ^= 1;

#pragma omp parallel for num_threads(4)
		for (int j = 0; j < loops; j++)
		{
			for (int k = 0; k < size_over_2; k++)
			{
				fft->c[fft->which][size * j + k] = fft->c[fft->which ^ 1][size * j + k] + fft->c[fft->which ^ 1][size * j + size_over_2 + k] * fft->W[w_][k];
			}
			for (int k = size_over_2; k < size; k++)
			{
				fft->c[fft->which][size * j + k] = fft->c[fft->which ^ 1][size * j - size_over_2 + k] - fft->c[fft->which ^ 1][size * j + k] * fft->W[w_][k - size_over_2];
			}
		}

		loops >>= 1;
		size <<= 1;
		size_over_2 <<= 1;
		w_++;
	}

	for (int i = 0; i < fft->N; i++)
		output[i * stride + offset] = fft->c[fft->which][i];
}

// -- ocean -- //

#define GRAVITY 9.81

struct Ocean_Vertex
{
	float x, y, z;    // vertex
	float nx, ny, nz; // normal
	float a, b, c;    // htilde0
	float _a, _b, _c; // htilde0mk conjugate
	float ox, oy, oz; // original position
};

struct Ocean
{
	int N;            // dimension -- N should be a power of 2
	float A;          // phillips spectrum parameter -- affects heights of waves
	vec2 w;           // wind parameter
	float length;     // length parameter
	Complex *h_tilde; // for fast fourier transform
	Complex *h_tilde_slopex, *h_tilde_slopez, *h_tilde_dx, *h_tilde_dz;
	FFT fft; // fast fourier transform
	Ocean_Vertex* vertices; // vertices info for simulation
};

Complex gaussianRandomVariable()
{
	float x1, x2, w;
	do {
		x1 = 2.f * random_chance() - 1.f;
		x2 = 2.f * random_chance() - 1.f;
		w = x1 * x1 + x2 * x2;
	} while (w >= 1.f);
	w = sqrt((-2.f * log(w)) / w);
	return Complex(x1 * w, x2 * w);
}
float phillips(Ocean ocean, int n_prime, int m_prime)
{
	vec2 k = vec2(PI * (2 * n_prime - ocean.N) / ocean.length, PI * (2 * m_prime - ocean.N) / ocean.length);
	float k_length = length(k);
	if (k_length < 0.000001) { return 0.0; }

	float k_length2 = k_length * k_length;
	float k_length4 = k_length2 * k_length2;

	float k_dot_w = dot(normalize(k), normalize(ocean.w));
	float k_dot_w2 = k_dot_w * k_dot_w;

	float w_length = length(ocean.w);
	float L = w_length * w_length / GRAVITY;
	float L2 = L * L;

	float damping = 0.001;
	float l2 = L2 * damping * damping;

	return ocean.A * exp(-1.0f / (k_length2 * L2)) / k_length4 * k_dot_w2 * exp(-k_length2 * l2);
}
float dispersion(uint N, float length, int n_prime, int m_prime)
{
	float w_0 = 2.0f * PI / 50.0f;
	float kx = PI * (2 * n_prime - N) / length;
	float kz = PI * (2 * m_prime - N) / length;
	return floor(sqrt(GRAVITY * sqrt(kx * kx + kz * kz)) / w_0) * w_0;
}
Complex hTilde_0(Ocean ocean, int n_prime, int m_prime)
{
	Complex r = gaussianRandomVariable();
	return r * sqrt(phillips(ocean, n_prime, m_prime) / 2.0);
}
Complex hTilde(Ocean ocean, float t, int n_prime, int m_prime)
{
	int index = m_prime * (ocean.N + 1) + n_prime;

	Complex htilde0 = Complex(ocean.vertices[index].a, ocean.vertices[index].b);
	Complex htilde0mkconj = Complex(ocean.vertices[index]._a, ocean.vertices[index]._b);

	float omegat = dispersion(ocean.N, ocean.length, n_prime, m_prime) * t;

	float cos_ = cos(omegat);
	float sin_ = sin(omegat);

	Complex c0 = Complex(cos_, sin_);
	Complex c1 = Complex(cos_, -sin_);

	Complex res = htilde0 * c0 + htilde0mkconj * c1;

	return htilde0 * c0 + htilde0mkconj * c1;
}

void init(Ocean* ocean, const int N, const float A, const vec2 w, const float length)
{
	*ocean = {};
	ocean->N = N;
	ocean->A = A;
	ocean->w = w;
	ocean->length = length;

	ocean->h_tilde        = Alloc(Complex, N * N);
	ocean->h_tilde_slopex = Alloc(Complex, N * N);
	ocean->h_tilde_slopez = Alloc(Complex, N * N);
	ocean->h_tilde_dx     = Alloc(Complex, N * N);
	ocean->h_tilde_dz     = Alloc(Complex, N * N);
	ocean->vertices       = Alloc(Ocean_Vertex, (N + 1) * (N + 1));

	init(&ocean->fft, N);

	int index;
	Complex htilde0, htilde0mk_conj;

	for (int m = 0; m < N + 1; m++) {
	for (int n = 0; n < N + 1; n++)
	{
		index = m * (N + 1) + n;

		htilde0 = hTilde_0(*ocean, n, m);
		std::complex<double> h0 = hTilde_0(*ocean, -n, -m);
		htilde0mk_conj = std::conj(h0);

		ocean->vertices[index].a = htilde0.real();
		ocean->vertices[index].b = htilde0.imag();
		ocean->vertices[index]._a = htilde0mk_conj.real();
		ocean->vertices[index]._b = htilde0mk_conj.imag();

		ocean->vertices[index].ox = ocean->vertices[index].x = (n - N / 2.0f) * length / N;
		ocean->vertices[index].oy = ocean->vertices[index].y = 0.0f;
		ocean->vertices[index].oz = ocean->vertices[index].z = (m - N / 2.0f) * length / N;

		ocean->vertices[index].nx = 0.0f;
		ocean->vertices[index].ny = 1.0f;
		ocean->vertices[index].nz = 0.0f;
	} }
}
void evaluate_waves(Ocean& ocean, float t) {
	float kx, kz, len, lambda = -1.0f;
	int index, index1;

	for (int m_prime = 0; m_prime < ocean.N; m_prime++) {
	for (int n_prime = 0; n_prime < ocean.N; n_prime++)
	{
		kz = PI * (2 * m_prime - ocean.N) / ocean.length;
		kx = PI * (2 * n_prime - ocean.N) / ocean.length;
		len = sqrt(kx * kx + kz * kz);
		index = m_prime * ocean.N + n_prime;

		ocean.h_tilde[index] = hTilde(ocean, t, n_prime, m_prime);

		ocean.h_tilde_slopex[index] = ocean.h_tilde[index] * Complex(0, kx);
		ocean.h_tilde_slopez[index] = ocean.h_tilde[index] * Complex(0, kz);
		if (len < 0.000001f)
		{
			ocean.h_tilde_dx[index] = Complex(0);
			ocean.h_tilde_dz[index] = Complex(0);
		}
		else
		{
			ocean.h_tilde_dx[index] = ocean.h_tilde[index] * Complex(0, -kx / len);
			ocean.h_tilde_dz[index] = ocean.h_tilde[index] * Complex(0, -kz / len);
		}
	} }

	for (int m_prime = 0; m_prime < ocean.N; m_prime++)
	{
		fast_fourier_transform(&ocean.fft, ocean.h_tilde       , ocean.h_tilde       , 1, m_prime * ocean.N);
		fast_fourier_transform(&ocean.fft, ocean.h_tilde_slopex, ocean.h_tilde_slopex, 1, m_prime * ocean.N);
		fast_fourier_transform(&ocean.fft, ocean.h_tilde_slopez, ocean.h_tilde_slopez, 1, m_prime * ocean.N);
		fast_fourier_transform(&ocean.fft, ocean.h_tilde_dx    , ocean.h_tilde_dx    , 1, m_prime * ocean.N);
		fast_fourier_transform(&ocean.fft, ocean.h_tilde_dz    , ocean.h_tilde_dz    , 1, m_prime * ocean.N);
	}

	for (int n_prime = 0; n_prime < ocean.N; n_prime++)
	{
		fast_fourier_transform(&ocean.fft, ocean.h_tilde       , ocean.h_tilde       , ocean.N, n_prime);
		fast_fourier_transform(&ocean.fft, ocean.h_tilde_slopex, ocean.h_tilde_slopex, ocean.N, n_prime);
		fast_fourier_transform(&ocean.fft, ocean.h_tilde_slopez, ocean.h_tilde_slopez, ocean.N, n_prime);
		fast_fourier_transform(&ocean.fft, ocean.h_tilde_dx    , ocean.h_tilde_dx    , ocean.N, n_prime);
		fast_fourier_transform(&ocean.fft, ocean.h_tilde_dz    , ocean.h_tilde_dz    , ocean.N, n_prime);
	}

	float signs[] = { 1.0f, -1.0f };

	for (int m_prime = 0; m_prime < ocean.N; m_prime++) {
	for (int n_prime = 0; n_prime < ocean.N; n_prime++)
	{
		index  = m_prime *    ocean.N    + n_prime; // index into h_tilde 
		index1 = m_prime * (ocean.N + 1) + n_prime; // index into vertices

		int sign = signs[(n_prime + m_prime) & 1];

		ocean.h_tilde[index] = ocean.h_tilde[index] * double(sign);

		// height
		ocean.vertices[index1].y = ocean.h_tilde[index].real();

		// displacement
		ocean.h_tilde_dx[index]  = ocean.h_tilde_dx[index] * double(sign);
		ocean.h_tilde_dz[index]  = ocean.h_tilde_dz[index] * double(sign);
		ocean.vertices[index1].x = ocean.vertices[index1].ox + ocean.h_tilde_dx[index].real() * lambda;
		ocean.vertices[index1].z = ocean.vertices[index1].oz + ocean.h_tilde_dz[index].real() * lambda;

		// normal
		ocean.h_tilde_slopex[index] = ocean.h_tilde_slopex[index] * double(sign);
		ocean.h_tilde_slopez[index] = ocean.h_tilde_slopez[index] * double(sign);
		vec3 n = normalize(vec3(-1 * ocean.h_tilde_slopex[index].real(), 1, -1 * ocean.h_tilde_slopez[index].real()));
		ocean.vertices[index1].nx = n.x;
		ocean.vertices[index1].ny = n.y;
		ocean.vertices[index1].nz = n.z;

		// for tiling
		if (n_prime == 0 && m_prime == 0)
		{
			ocean.vertices[index1 + ocean.N + (ocean.N + 1) * ocean.N].y  = ocean.h_tilde[index].real();
			ocean.vertices[index1 + ocean.N + (ocean.N + 1) * ocean.N].x  = ocean.vertices[index1 + ocean.N + (ocean.N + 1) * ocean.N].ox + ocean.h_tilde_dx[index].real() * lambda;
			ocean.vertices[index1 + ocean.N + (ocean.N + 1) * ocean.N].z  = ocean.vertices[index1 + ocean.N + (ocean.N + 1) * ocean.N].oz + ocean.h_tilde_dz[index].real() * lambda;
			ocean.vertices[index1 + ocean.N + (ocean.N + 1) * ocean.N].nx = n.x;
			ocean.vertices[index1 + ocean.N + (ocean.N + 1) * ocean.N].ny = n.y;
			ocean.vertices[index1 + ocean.N + (ocean.N + 1) * ocean.N].nz = n.z;
		}
		if (n_prime == 0)
		{
			ocean.vertices[index1 + ocean.N].y  = ocean.h_tilde[index].real();
			ocean.vertices[index1 + ocean.N].x  = ocean.vertices[index1 + ocean.N].ox + ocean.h_tilde_dx[index].real() * lambda;
			ocean.vertices[index1 + ocean.N].z  = ocean.vertices[index1 + ocean.N].oz + ocean.h_tilde_dz[index].real() * lambda;
			ocean.vertices[index1 + ocean.N].nx = n.x;
			ocean.vertices[index1 + ocean.N].ny = n.y;
			ocean.vertices[index1 + ocean.N].nz = n.z;
		}
		if (m_prime == 0)
		{
			ocean.vertices[index1 + (ocean.N + 1) * ocean.N].y  = ocean.h_tilde[index].real();
			ocean.vertices[index1 + (ocean.N + 1) * ocean.N].x  = ocean.vertices[index1 + (ocean.N + 1) * ocean.N].ox + ocean.h_tilde_dx[index].real() * lambda;
			ocean.vertices[index1 + (ocean.N + 1) * ocean.N].z  = ocean.vertices[index1 + (ocean.N + 1) * ocean.N].oz + ocean.h_tilde_dz[index].real() * lambda;
			ocean.vertices[index1 + (ocean.N + 1) * ocean.N].nx = n.x;
			ocean.vertices[index1 + (ocean.N + 1) * ocean.N].ny = n.y;
			ocean.vertices[index1 + (ocean.N + 1) * ocean.N].nz = n.z;
		}
	} }
}

void write_heightmap(Ocean ocean)
{
	int w, h;
	w = h = ocean.N;

	bvec3* bitmap = (bvec3*)calloc(w * h, 3); // 24-bits = 3-bytes = RGB
	bvec3 color;

	if (!bitmap) { out("ERROR: Cannot allocate heightmap!"); exit(EXIT_FAILURE); }

	// saving height & xz-displacement in one image
	// (R, G, B) <-- (value + offset) / scale * 255
	// (R, G, B) is for (x-displacement, height, z-displacement)

	for (int i = 0; i < w; i++) {
	for (int j = 0; j < h; j++)
	{
		int index = (i * w) + j;

		float x = ocean.vertices[index].ox - ocean.vertices[index].x;
		float y = ocean.vertices[index].oy - ocean.vertices[index].y;
		float z = ocean.vertices[index].oz - ocean.vertices[index].z;

		x = (x + 2.0) / 10.0 * 255.0;
		y = (y + 2.0) / 10.0 * 255.0;
		z = (z + 2.0) / 10.0 * 255.0;

		color.r = int(x);
		color.g = int(y);
		color.b = int(z);

		bitmap[(i * w) + j] = color;
	} }

	stbi_write_bmp("height.bmp", w, h, 3, bitmap);
}
void write_normal_map(Ocean ocean)
{
	int w, h;
	w = h = ocean.N;

	bvec3* bitmap = (bvec3*)calloc(w * h, 3); // 24-bits = 3-bytes = RGB
	bvec3 color;

	if (!bitmap) { out("ERROR : Cannot allocate normal image buffer!"); exit(EXIT_FAILURE); }

	for (int i = 0; i < w; i++) {
	for (int j = 0; j < h; j++)
	{
		int idx = i * ocean.N + j;

		vec3 normal(ocean.vertices[idx].nx, ocean.vertices[idx].ny, ocean.vertices[idx].nz);
		normal = normalize(normal);      // to [-1, 1]
		normal = (normal + 1.0f) / 2.0f; // to [0, 1]

		// to [0, 255]
		color.r = byte(normal.x * 255.0);
		color.g = byte(normal.y * 255.0);
		color.b = byte(normal.z * 255.0);

		bitmap[(i * w) + j] = color;
	} }

	stbi_write_bmp("normal.bmp", w, h, 3, (byte*)bitmap);
}
void write_folding_map(Ocean ocean)
{
	int w, h;
	w = h = ocean.N;

	bvec3* bitmap = (bvec3*)calloc(w * h, 3); // 24-bits = 3-bytes = RGB
	bvec3 color;

	if (!bitmap) {
		std::cout << "FreeImage: Cannot allocate image." << '\n';
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < w; i++) {
		for (int j = 0; j < h; j++)
		{
			// index for center finite difference
			int iLeft = (i - 1 < 0) ? w - 1 : i - 1;
			int iRight = (i + 1) % w;
			int jUp = (j - 1 < 0) ? h - 1 : j - 1;
			int jDown = (j + 1) % h;

			int idxLeft = iLeft * ocean.N + j;
			int idxRight = iRight * ocean.N + j;
			int idxUp = i * ocean.N + jUp;
			int idxDown = i * ocean.N + jDown;

			// x, z displacement
			// lambda has already been multiplied in evaluateWavesFFT()
			float Dx_xplus1 = ocean.vertices[idxRight].ox - ocean.vertices[idxRight].x;
			float Dx_xminus1 = ocean.vertices[idxLeft].ox - ocean.vertices[idxLeft].x;

			float Dz_zplus1 = ocean.vertices[idxUp].oz - ocean.vertices[idxUp].z;
			float Dz_zminus1 = ocean.vertices[idxDown].oz - ocean.vertices[idxDown].z;

			// Jacobian (or folding map)
			float Jxx = 1.f + (Dx_xplus1 - Dx_xminus1) * 0.5f;
			float Jzz = 1.f + (Dz_zplus1 - Dz_zminus1) * 0.5f;
			float Jzx = (Dz_zplus1 - Dz_zminus1) * 0.5f;
			float Jxz = Jzx;

			float Jacob = Jxx * Jzz - Jxz * Jzx;
			float epsilon = 0.9f;

			if (Jacob - epsilon > 0)
			{
				Jacob = Jacob - epsilon;
			}
			else Jacob = 0;

			// std::cout << Jacob << '\n';

			// convert to RGB
			Jacob = (Jacob * 1.f) * 255.f;
			if (Jacob < 255.f)
			{
				Jacob = Jacob;
			}
			else Jacob = 255.f;
			// std::cout << Jacob << '\n';

			color.r = int(Jacob);
			color.g = int(Jacob);
			color.b = int(Jacob);

			bitmap[(i * w) + j] = color;
		}
	}

	stbi_write_bmp("foam.bmp", w, h, 3, bitmap);
}