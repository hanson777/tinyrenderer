#include "model.h"
#include "tgaimage.h"
#include <iostream>
#include <string>
#include <utility>

constexpr int width = 800;
constexpr int height = 800;

constexpr TGAColor white = { 255, 255, 255, 255 }; // attention, BGRA order
constexpr TGAColor green = { 0, 255, 0, 255 };
constexpr TGAColor red = { 0, 0, 255, 255 };
constexpr TGAColor blue = { 255, 128, 64, 255 };
constexpr TGAColor yellow = { 0, 200, 255, 255 };

void line(int ax, int ay, int bx, int by, TGAImage& framebuffer, const TGAColor& color) {
    bool steep = std::abs(ax - bx) < std::abs(ay - by);
    if (steep) {
        std::swap(ax, ay);
        std::swap(bx, by);
    }
    if (ax > bx) {
        std::swap(ax, bx);
        std::swap(ay, by);
    }
    float y = ay;
    for (int x = ax; x <= bx; x++) {
        if (steep) // mirror again if mirrored previously
            framebuffer.set(y, x, color);
        else framebuffer.set(x, y, color);
        y = y + (by - ay) / static_cast<float>(bx - ax);
    }
}

double signed_triangle_area(int ax, int ay, int bx, int by, int cx, int cy) {
    return 0.5 * ((bx - ax) * (cy - ay) - (by - ay) * (cx - ax));
}

void triangle(vec3 a, vec3 b, vec3 c, TGAImage& zbuffer, TGAImage& framebuffer, TGAColor color) {
    double total_area = signed_triangle_area(a.x, a.y, b.x, b.y, c.x, c.y);
    if (total_area < 1) return; // backface culling + discarding triangles that cover less than a pixel

	int bbmin_x = std::min({ a.x, b.x, c.x });
	int bbmax_x = std::max({ a.x, b.x, c.x });
	int bbmin_y = std::min({ a.y, b.y, c.y });
	int bbmax_y = std::max({ a.y, b.y, c.y });

    for (int x = bbmin_x; x <= bbmax_x; x++) {
        for (int y = bbmin_y; y <= bbmax_y; y++) {
            double alpha = signed_triangle_area(x, y, b.x, b.y, c.x, c.y) / total_area; // we don't need to divide by total_area if it's a sign check 
            double beta = signed_triangle_area(x, y, c.x, c.y, a.x, a.y) / total_area;  // i left it here cause we use it for the color
            double gamma = signed_triangle_area(x, y, a.x, a.y, b.x, b.y) / total_area;
			if (alpha < 0 || beta < 0 || gamma < 0) continue; // negative barycentric coordinate -> the pixel is outside the triangle
            auto z = static_cast<unsigned char>(alpha * a.z + beta * b.z + gamma * c.z);
            if (z <= zbuffer.get(x, y)[0]) continue;
            zbuffer.set(x, y, { z });
            framebuffer.set(x, y, color);
        }
    }
}

std::tuple<float, float, float> viewport_transform(vec3 v) {
    float x = (v.x + 1.) * (width / 2.0f);
    float y = (v.y + 1.) * (height / 2.0f);
    float z = (v.z + 1.) * (255 / 2.0f);
    return std::make_tuple(x, y, z);
}

void draw_model(Model& model, TGAImage& zbuffer, TGAImage& framebuffer) {
    std::cout << "# Vertices: " << model.nvertices() << std::endl;
    std::cout << "# Faces: " << model.nfaces() << std::endl;

    for (int i = 0; i < model.nfaces(); i++) {
        auto [ax, ay, az] = viewport_transform(model.vertices(i, 0));
        auto [bx, by, bz] = viewport_transform(model.vertices(i, 1));
        auto [cx, cy, cz] = viewport_transform(model.vertices(i, 2));
		TGAColor rnd;
		for (int c : {0,1,2}) rnd[c] = std::rand() % 255;
        triangle({ ax,ay,az }, { bx,by,bz }, { cx,cy,cz }, zbuffer, framebuffer, rnd);
    }

    //for (int i = 0; i < model.nvertices(); i++) {
    //    auto [x, y] = viewport_transform(model.vertices(i));
    //    framebuffer.set(x, y, white);
    //}
}

int main(int argc, char** argv) {
    std::cout << "We are all alone on life's journey, held captive by the limitations of human consciousness.\n";
    // if (argc != 2) {
    //     std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
    //     return 1;
    // }

    Model model("obj/diablo3_pose/diablo3_pose.obj");
	TGAImage framebuffer(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

    draw_model(model, zbuffer, framebuffer);

    framebuffer.write_tga_file("framebuffer.tga");
    zbuffer.write_tga_file("zbuffer.tga");
    return 0;
}
