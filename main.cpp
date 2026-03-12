#include "our_gl.h"
#include "model.h"
#include "tgaimage.h"
#include <iostream>
#include <string>
#include <array>
#include <cmath>

extern mat<4,4> ModelView, Perspective; // "OpenGL" state matrices and
extern std::vector<double> zbuffer;     // the depth buffer

constexpr double PI = 3.14159265358979323846;

constexpr TGAColor white = { {255, 255, 255, 255} }; // BGRA
constexpr TGAColor green = { {0, 255, 0, 255} };
constexpr TGAColor red = { {0, 0, 255, 255} };
constexpr TGAColor blue = { {255, 128, 64, 255 }};
constexpr TGAColor yellow = {{ 0, 200, 255, 255 }};

struct PhongShader : IShader {
	const Model& model;
	TGAColor color{};
	std::array<vec3, 3> tri; // triangle in eye coordinates
	std::array<vec3, 3> norm;

	PhongShader(const Model& m) : model(m) {}

	virtual vec4 vertex(const int face, const int vert) {
		const auto [x, y, z] = model.vertices(face, vert);                               // current vertex in object coordinates
		const vec4 gl_Position = ModelView * vec4{x, y, z, 1.};
		tri[vert] = gl_Position.xyz(); // in eye coordinates
		const vec3 n = model.normals(face, vert);
		norm[vert] = (ModelView.invert_transpose() * vec4{n.x, n.y, n.z, 0.}).xyz();
		return Perspective * gl_Position;                                              // in clip coordinates
	}

	std::pair<bool,TGAColor> fragment(const vec3 bar) const override {
		const auto [A, B, C] = tri;

		constexpr double ambient = 0.3;

		// diffuse
		const vec3 n = normalized(bar.x*norm[0] + bar.y*norm[1] + bar.z*norm[2]); // alpha_n = bar.n
		// const vec3 n = normalized(cross(B-A, C-A));
		const vec3 l = normalized(vec3{1, 0, 0});
		const double diffuse = std::max(0.0f, static_cast<float>(n*l));
		// end diffuse

		// specular
		const vec3 r = normalized(2*n*(n*l) - l);
		const vec3 v = normalized(vec3{-1,0,2});
		const double specular = std::pow(std::max<float>(0.0f, r*v), 32); // last param is the 'shiny' term (e)
		// end specular

		double intensity = ambient + 0.4*diffuse + 0.9*specular;
		intensity = std::min(1.0, intensity);

		TGAColor c = {
			{
				static_cast<uint8_t>(255*intensity), static_cast<uint8_t>(255*intensity),
	 static_cast<uint8_t>(255*intensity), 255
			}};
		return {false, c}; // do not discard the pixel
	}
};

void draw_model(const Model& model, std::vector<double>& zbuffer, TGAImage& framebuffer) {
    std::cout << "Vertices: " << model.nvertices() << '\n';
    std::cout << "Faces: " << model.nfaces() << '\n';

	PhongShader shader(model);
    for (int i = 0; i < model.nfaces(); i++) {
    	Triangle clip = {shader.vertex(i, 0), shader.vertex(i, 1), shader.vertex(i, 2)};
        rasterize(clip, shader, framebuffer);
    }
    std::cout << "All pixels drawn.\n";
}

int main(int argc, char** argv) {
    std::cout << "We are all alone on life's journey, held captive by the limitations of human consciousness.\n";

	constexpr int width = 800;
	constexpr int height = 800;
	constexpr vec3    eye{ -1,0,2 }; // camera position
	constexpr vec3 center{ 0,0,0 };  // camera direction
	constexpr vec3     up{ 0,1,0 };  // camera up vector

	lookat(eye, center, up);                                               // build the ModelView   matrix
	init_perspective(norm(eye - center));                                  // build the Perspective matrix
	init_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8); // build the Viewport    matrix
	init_zbuffer(width, height);

    const Model model("obj/diablo3_pose/diablo3_pose.obj");
	//TGAImage framebuffer(width, height, TGAImage::RGB, {177, 195, 209, 255});
	TGAImage framebuffer(width, height, TGAImage::RGB);

    std::vector<double> zbuffer(width * height, -std::numeric_limits<double>::max());

    draw_model(model, zbuffer, framebuffer);

    assert(framebuffer.write_tga_file("framebuffer.tga"));
    return 0;
}
