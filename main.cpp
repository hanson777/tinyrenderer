#define _USE_MATH_DEFINES
#include "our_gl.h"
#include "model.h"
#include "tgaimage.h"
#include <iostream>
#include <string>
#include <array>
#include <cmath>
#include <algorithm>
#include <random>

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define BOLD    "\033[1m"

extern mat<4,4> ModelView, Viewport, Perspective; // "OpenGL" state matrices and
extern std::vector<double> zbuffer;     // the depth buffer
std::vector<double> shadowmap;

float lerp(float a, float b, float t) {
    return a + t * (b-a);
}

float clamp(float x, float lowerlimit = 0.0f, float upperlimit = 1.0f) {
  if (x < lowerlimit) return lowerlimit;
  if (x > upperlimit) return upperlimit;
  return x;
}

float smoothstep (float edge0, float edge1, float x) {
   // Scale, and clamp x to 0..1 range
   x = clamp((x - edge0) / (edge1 - edge0));

   return x * x * (3.0f - 2.0f * x);
}

struct BlankShader : IShader {
	const Model& model;

	BlankShader(const Model& m) : model(m) {}

	virtual vec4 vertex(const int face, const int vert) {
		const vec4 gl_Position = ModelView * model.vert(face, vert);
		return Perspective * gl_Position;
	}

	std::pair<bool, TGAColor> fragment(const vec3 bar) const override {
		return { false, {255, 255, 255, 255} };
	}
};

struct PhongShader : IShader {
	const Model& model;
	TGAColor color{};
	std::array<vec3, 3> tri; // triangle in eye coordinates
	std::array<vec3, 3> norm;
	std::array<vec2, 3> uvs;
    vec4 l;
	mat<4, 4> NM;
    int m_sampleSize;
    std::vector<vec3> m_kernel;

	PhongShader(const Model& m, const vec3 light, mat<4,4> trans) : model(m) {
		l = normalized((ModelView * vec4{ light.x, light.y, light.z, 0. }));
		NM = trans;
        int sampleSize = 128;
        std::random_device rd; // gives us the seed when we call rd() 
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.0, 1.0); // random floats from [0,1)
        std::vector<vec3> kernel(sampleSize);
        for (int i = 0; i < sampleSize; i++) {
            vec3 sample{ dist(gen) * 2. - 1., dist(gen) * 2. - 1., dist(gen) };
            sample = normalized(sample);
            sample = sample * dist(gen);
            float scale = (float)i / sampleSize;
            scale = lerp(.1, 1., scale * scale);
            sample = sample * scale;
            kernel.push_back(sample);
        }
        m_sampleSize = sampleSize;
        m_kernel = kernel;
    }

	virtual vec4 vertex(const int face, const int vert) {
		const vec4 gl_Position = ModelView * model.vert(face, vert);
		tri[vert] = gl_Position.xyz(); // in eye coordinates
		const vec4 n = model.normal(face, vert);
		norm[vert] = (ModelView.invert_transpose() * vec4{n.x, n.y, n.z, n.w}).xyz();
		uvs[vert] = model.uv(face, vert);
		return Perspective * gl_Position;                                              // in clip coordinates
	}

	std::pair<bool,TGAColor> fragment(const vec3 bar) const override {

        // grab uv
		vec2 uv = uvs[0] * bar[0] + uvs[1] * bar[1] + uvs[2] * bar[2];
        
        // normal tangent mapping
        vec3 e0 = tri[1] - tri[0];
        vec3 e1 = tri[2] - tri[0];
        vec2 u0 = uvs[1] - uvs[0];
        vec2 u1 = uvs[2] - uvs[0];

        mat<2, 4> E = {{{e0.x, e0.y, e0.z}, {e1.x,e1.y, e1.z}}};
        mat<2,2> U = {{{u0.x, u0.y}, {u1.x, u1.y}}};
        mat<2,4> tb = U.invert() * E;
        vec4 t = normalized(tb[0]);
        vec4 b = normalized(tb[1]);
        vec3 n = normalized(norm[0]*bar.x + norm[1]*bar.y + norm[2]*bar.z);

        mat<4,4> TBN = {{t, b, {n.x, n.y, n.z, 0}, {0,0,0,1}}};
        vec4 n_world = normalized(TBN.transpose() * model.normal(uv));
        
		// diffuse
		const double diffuse = std::max(0.0, n_world*l);

		// specular
		const vec4 r = normalized(2*n_world*(n_world*l) - l);
		double specular = sample2D(model.specular(), uv)[0]/255. * std::pow(std::max(r.z, 0.), 35);

		double ambient = 0.15;

        vec3 frag = tri[0] * bar.x + tri[1] * bar.y + tri[2] * bar.z; 
        // std::cout << "fragPos: " << frag << "\n";

        std::random_device rd; // gives us the seed when we call rd() 
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.0, 1.0); // random floats from [0,1)
        vec3 randomVec = vec3{dist(gen), dist(gen), dist(gen)};
        float radius = .4;
        float occlusion = 0.;
        vec3 tangent = normalized(randomVec - n * (randomVec * n));
        vec3 bitangent = cross(n, tangent);
        mat<3,3> tbn = {tangent, bitangent, n};

        for (int i = 0; i < m_kernel.size(); i++) {
            vec3 samplePos = tbn * m_kernel[i]; // tangent normal to view-space
            vec4 clip = Perspective * vec4{samplePos.x, samplePos.y, samplePos.z, 1.}; // view -> clip
            vec4 ndc = clip; // perspective divide
            ndc.x = ndc.x / ndc.w;
            ndc.y = ndc.y / ndc.w;
            ndc.z = ndc.z / ndc.w;
            ndc.x = ndc.x * 0.5 + 0.5;
            ndc.y = ndc.y * 0.5 + 0.5;
            ndc.z = ndc.z * 0.5 + 0.5;
            vec2 screen = (Viewport * ndc).xy(); // our uv
            int x = std::clamp((int)(screen.x*800), 0, 800);
            int y = std::clamp((int)(screen.y*800), 0, 800);
            double sampleDepth = zbuffer[x + y * 800];
            float rangeCheck = smoothstep(0., 1., radius / std::abs(frag.z - sampleDepth));
            occlusion += (sampleDepth >= samplePos.z + 0.005 ? 1. : 0.) * rangeCheck;
        }
        occlusion = 1. - (occlusion / m_kernel.size());

		vec4 shadow_coord = NM * vec4{ frag.x, frag.y, frag.z, 1 };
		   
		float i = shadow_coord.x / shadow_coord.w;
		float j = shadow_coord.y / shadow_coord.w;
		float z = shadow_coord.z / shadow_coord.w;

		int si = std::clamp((int)i, 0, 800 - 1);
		int sj = std::clamp((int)j, 0, 800 - 1);
		bool in_shadow = z < shadowmap[si + sj * 800] - 0.005; // bias

        ambient *= occlusion;
		double intensity = std::min(1.0, ambient + diffuse + specular);
		if (in_shadow) intensity = ambient;

		TGAColor gl_FragColor = sample2D(model.diffuse(), uv);
		for (const int c : {0, 1, 2}) gl_FragColor[c] = std::min<int>(255, gl_FragColor[c] * intensity);
		return {false, gl_FragColor}; // do not discard the pixel
	}
};

void drop_zbuffer(std::string filename, std::vector<double>& zbuffer, int width, int height) {
	TGAImage zimg(width, height, TGAImage::GRAYSCALE, { 0,0,0,0 });
	double minz = +1000;
	double maxz = -1000;
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			double z = zbuffer[x + y * width];
			if (z < -100) continue;
			minz = std::min(z, minz);
			maxz = std::max(z, maxz);
		}
	}
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			double z = zbuffer[x + y * width];
			if (z < -100) continue;
			z = (z - minz) / (maxz - minz) * 255;
			zimg.set(x, y, { (uint8_t)z, 255, 255, 255 });
		}
	}
	zimg.write_tga_file(filename);
}


int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
        return 1;
    }	

    std::cout << "We are all alone on life's journey, held captive by the limitations of human consciousness.\n";
	//std::cout << YELLOW << "These cigarettes... have a sweet aftertaste.\nMy life... never had dessert.\n" << RESET;

	constexpr int width = 800; // image size
	constexpr int height = 800;
	constexpr vec3    eye{ -1,0,2 }; // camera position
	constexpr vec3 center{ 0,0,0 };  // camera direction
	constexpr vec3     up{ 0,1,0 };  // camera up vector
    constexpr vec3 light{1, 1, 1};   // light position 
	TGAImage framebuffer(width, height, TGAImage::RGB, { 177, 195, 209, 255 });

	// render from light's perspective
	lookat(light, center, up);
	init_perspective(norm(light - center));
	init_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);
	init_zbuffer(width, height);
	TGAImage shadow(width, height, TGAImage::RGB);
	for (int i = 0; i < argc; i++) {
        Model model(argv[i]);
		BlankShader shader(model);
		for (int i = 0; i < model.nfaces(); i++) {
			Triangle clip = { shader.vertex(i, 0), shader.vertex(i, 1), shader.vertex(i, 2) };
			rasterize(clip, shader, shadow);
		}
	}
	shadowmap = zbuffer;

	drop_zbuffer("shadowmap.tga", shadowmap, 800, 800);

	// render from camera perspective
	mat<4, 4> M_light = Viewport * Perspective * ModelView;
	lookat(eye, center, up);
	init_perspective(norm(eye - center));
	init_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);
	init_zbuffer(width, height);
	mat<4, 4> MN = M_light * ModelView.invert();

	for (int i = 0; i < argc; i++) {
        Model model(argv[i]);
		PhongShader phong(model, light, MN);
		for (int i = 0; i < model.nfaces(); i++) {
			Triangle clip = { phong.vertex(i, 0), phong.vertex(i, 1), phong.vertex(i, 2) };
			rasterize(clip, phong, framebuffer);
		}
	}

	framebuffer.write_tga_file("framebuffer.tga");
	return 0;
}
