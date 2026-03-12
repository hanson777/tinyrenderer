#define _USE_MATH_DEFINES
#include "our_gl.h"
#include "model.h"
#include "tgaimage.h"
#include <iostream>
#include <string>
#include <array>
#include <cmath>

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define BOLD    "\033[1m"

extern mat<4,4> ModelView, Perspective; // "OpenGL" state matrices and
extern std::vector<double> zbuffer;     // the depth buffer

struct PhongShader : IShader {
	const Model& model;
	TGAColor color{};
	std::array<vec3, 3> tri; // triangle in eye coordinates
	std::array<vec3, 3> norm;
	std::array<vec2, 3> uvs;
    const vec4 l;

	PhongShader(const Model& m, const vec4 light_dir) : model(m), l(normalized(light_dir)) {
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

		constexpr double ambient = 0.15;
		double intensity = std::min(1.0, ambient + diffuse + specular);

		TGAColor gl_FragColor = sample2D(model.diffuse(), uv);
		for (const int c : {0, 1, 2}) gl_FragColor[c] = std::min<int>(255, gl_FragColor[c] * intensity);
		return {false, gl_FragColor}; // do not discard the pixel
	}
};

void draw_model(const Model& model, TGAImage& framebuffer, const vec4& light_dir) {
    std::cout << model.nverts() << " vertices, " << model.nfaces() << " faces"<< '\n';

	PhongShader shader(model, light_dir);
    for (int i = 0; i < model.nfaces(); i++) {
    	Triangle clip = {shader.vertex(i, 0), shader.vertex(i, 1), shader.vertex(i, 2)};
        rasterize(clip, shader, framebuffer);
    }
}

int main(int argc, char** argv) {
	// std::cout << "We are all alone on life's journey, held captive by the limitations of human consciousness.\n";
    std::cout << YELLOW << "These cigarettes... have a sweet aftertaste.\nMy life... never had dessert.\n";
    std::cout << "Are you my dessert... Okkotsu?!\n" << RESET;

	constexpr int width = 800;
	constexpr int height = 800;
	constexpr vec3    eye{ -1,0,2 }; // camera position
	constexpr vec3 center{ 0,0,0 };  // camera direction
	constexpr vec3     up{ 0,1,0 };  // camera up vector
    constexpr vec4 light_dir{2, 1, 0, 0}; // Cruel Sun

	lookat(eye, center, up);                                               // build the ModelView   matrix
	init_perspective(norm(eye - center));                                  // build the Perspective matrix
	init_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8); // build the Viewport    matrix
	init_zbuffer(width, height);                                           // populate the zbuffer with -infinity
	TGAImage framebuffer(width, height, TGAImage::RGB);

	// std::string modelpath;
	// std::cout << "Enter model name: ";
	// std::cin >> modelpath;
	// const Model model("obj/" + modelpath + "/" + modelpath + ".obj");
	//TGAImage framebuffer(width, height, TGAImage::RGB, {177, 195, 209, 255});

	//draw_model(model, framebuffer);

	const std::vector<Model> models = {
		Model("obj/african_head/african_head.obj"),
		Model("obj/african_head/african_head_eye_outer.obj"),
		Model("obj/african_head/african_head_eye_inner.obj")
		// Model("obj/diablo3_pose/diablo3_pose.obj")
	 };
	for (const Model& m : models) draw_model(m, framebuffer, light_dir);

	bool success = framebuffer.write_tga_file("framebuffer.tga");
    if (!success) std::cerr << "file write failed";
    std::cout << GREEN << "Drawing complete.\n" << RESET;
	return 0;
}
