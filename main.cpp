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
	std::array<vec2, 3> uvs;

	PhongShader(const Model& m) : model(m) {}

	virtual vec4 vertex(const int face, const int vert) {
		const vec4 gl_Position = ModelView * model.vert(face, vert);
		tri[vert] = gl_Position.xyz(); // in eye coordinates
		const vec4 n = model.normal(face, vert);
		norm[vert] = (ModelView.invert_transpose() * vec4{n.x, n.y, n.z, n.w}).xyz();
		uvs[vert] = model.uv(face, vert);
		return Perspective * gl_Position;                                              // in clip coordinates
	}

	std::pair<bool,TGAColor> fragment(const vec3 bar) const override {
		const auto [A, B, C] = tri;

		constexpr double ambient = 0.4;

		// uv map stuff
		vec2 uv = uvs[0] * bar[0] + uvs[1] * bar[1] + uvs[2] * bar[2];
		vec4 n = normalized(ModelView.invert_transpose() * model.normal(uv));

		// diffuse
		//const vec3 n = normalized(bar.x*norm[0] + bar.y*norm[1] + bar.z*norm[2]); // alpha_n = bar.n
		//const vec3 n = normalized(cross(B-A, C-A));
		const vec4 l = normalized(vec4{1, 1, 1, 0});
		const double diffuse = std::max(0.0, n*l);

		// specular
		const vec4 r = normalized(2*n*(n*l) - l);
		//const double specular = std::pow(std::max(0.0, r.z), e); // last param is the 'shiny' term (e)
		double specular = sample2D(model.specular(), uv)[0]/255. * std::pow(std::max(r.z, 0.), 35);
		// end specular

		double intensity = std::min(1.0, ambient + diffuse + specular);

		TGAColor gl_FragColor = sample2D(model.diffuse(), uv);
		for (int channel : {0, 1, 2}) gl_FragColor[channel] = std::min<int>(255, gl_FragColor[channel] * intensity);
		return {false, gl_FragColor}; // do not discard the pixel
	}
};

void draw_model(const Model& model, TGAImage& framebuffer) {
    std::cout << "Vertices: " << model.nverts() << '\n';
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
	init_zbuffer(width, height);                                           // populate the zbuffer with -infinity
	TGAImage framebuffer(width, height, TGAImage::RGB);

	std::string modelpath;
	std::cout << "Enter model name: ";
	std::cin >> modelpath;
	const Model model("obj/" + modelpath + "/" + modelpath + ".obj");
	//TGAImage framebuffer(width, height, TGAImage::RGB, {177, 195, 209, 255});

	draw_model(model, framebuffer);
	
	/*std::vector<Model> models = {
	Model("obj/african_head/african_head.obj"),
	Model("obj/african_head/african_head_eye_outer.obj"),
	Model("obj/african_head/african_head_eye_inner.obj")
	};
	for (const Model& m : models) {
		draw_model(m, framebuffer);
	}*/

    assert(framebuffer.write_tga_file("framebuffer.tga"));
    return 0;
}
