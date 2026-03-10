#include "model.h"
#include "tgaimage.h"
#include <iostream>
#include <string>
#include <array>
#include <algorithm>

constexpr double PI = 3.14159265358979323846;

constexpr TGAColor white = { 255, 255, 255, 255 }; // BGRA
constexpr TGAColor green = { 0, 255, 0, 255 };
constexpr TGAColor red = { 0, 0, 255, 255 };
constexpr TGAColor blue = { 255, 128, 64, 255 };
constexpr TGAColor yellow = { 0, 200, 255, 255 };

mat<4, 4> ModelView, Viewport, Perspective;

double signed_triangle_area(int ax, int ay, int bx, int by, int cx, int cy) {
    return 0.5 * ((bx - ax) * (cy - ay) - (by - ay) * (cx - ax));
}

void lookat(const vec3 eye, const vec3 center, const vec3 up) {
	vec3 n = normalized(eye - center);
	vec3 l = normalized(cross(up, n));
	vec3 m = normalized(cross(n, l));
	ModelView = mat<4, 4>{ {{l.x,l.y,l.z,0}, {m.x,m.y,m.z,0}, {n.x,n.y,n.z,0}, {0,0,0,1}} } *
		mat<4, 4>{{{1, 0, 0, -center.x}, { 0,1,0,-center.y }, { 0,0,1,-center.z }, { 0,0,0,1 }}};
}

void perspective(const double f) {
	Perspective = { {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0, -1 / f,1}} };
}

void viewport(const int x, const int y, const int w, const int h) {
	Viewport = { {{w / 2., 0, 0, x + w / 2.}, {0, h / 2., 0, y + h / 2.}, {0,0,1,0}, {0,0,0,1}} };
}

void rasterize(const std::array<vec4, 3> clip, std::vector<double>& zbuffer, TGAImage& framebuffer, const TGAColor color) {
    std::array<vec4, 3> ndc = { clip[0] / clip[0].w, clip[1] / clip[1].w, clip[2] / clip[2].w };
    std::array<vec2, 3> screen = { (Viewport * ndc[0]).xy(), (Viewport * ndc[1]).xy(), (Viewport * ndc[2]).xy() }; // screen coordinates

	mat<3, 3> ABC = { { {screen[0].x, screen[0].y, 1.}, {screen[1].x, screen[1].y, 1.}, {screen[2].x, screen[2].y, 1.} } }; // det = 2x signed area
	if (ABC.det() < 1) return; // backface culling + discarding triangles that cover less than a pixel

    auto [bbmin_x, bbmax_x] = std::minmax({screen[0].x, screen[1].x, screen[2].x});
    auto [bbmin_y, bbmax_y] = std::minmax({screen[0].y, screen[1].y, screen[2].y });

    for (int x = bbmin_x; x <= bbmax_x; x++) {
        for (int y = bbmin_y; y <= bbmax_y; y++) {
			vec3 bc = ABC.invert_transpose() * vec3 { static_cast<double>(x), static_cast<double>(y), 1. }; // barycentric coordinates of {x,y} w.r.t the triangle
			if (bc.x < 0 || bc.y < 0 || bc.z < 0) continue; // negative barycentric coordinate -> the pixel is outside the triangle
            double z = bc * vec3{ ndc[0].z, ndc[1].z, ndc[2].z };
            if (z <= zbuffer[x + y * framebuffer.width()]) continue;
            zbuffer[x + y * framebuffer.width()] = z;
            framebuffer.set(x, y, color);
        }
    }
}

void draw_model(Model& model, std::vector<double>& zbuffer, TGAImage& framebuffer) {
    std::cout << "Vertices: " << model.nvertices() << '\n';
    std::cout << "Faces: " << model.nfaces() << '\n';

#pragma omp parallel for
    for (int i = 0; i < model.nfaces(); i++) {
        std::array<vec4, 3> clip;
        for (int d : {0, 1, 2}) {
            vec3 v = model.vertices(i, d);
            clip[d] = Perspective * ModelView * vec4{ v.x, v.y, v.z, 1. };
        }
		TGAColor rnd;
		for (int c : {0,1,2}) rnd[c] = std::rand() % 255;
        rasterize(clip, zbuffer, framebuffer, rnd);
    }
    std::cout << "All pixels drawn...\n";
}

int main(int argc, char** argv) {
    std::cout << "We are all alone on life's journey, held captive by the limitations of human consciousness.\n";

	constexpr int width = 800;
	constexpr int height = 800;
	constexpr vec3    eye{ -1,0,2 }; // camera position
	constexpr vec3 center{ 0,0,0 };  // camera direction
	constexpr vec3     up{ 0,1,0 };  // camera up vector

	lookat(eye, center, up);                                          // build the ModelView   matrix
	perspective(norm(eye - center));                                  // build the Perspective matrix
	viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8); // build the Viewport    matrix

    Model model("obj/diablo3_pose/diablo3_pose.obj");
	TGAImage framebuffer(width, height, TGAImage::RGB);

    std::vector<double> zbuffer(width * height, -std::numeric_limits<double>::max());

    draw_model(model, zbuffer, framebuffer);

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
