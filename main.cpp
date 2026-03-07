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

std::tuple<float, float> viewport_transform(vec3 v) {
    float x = (v.x + 1) * (width / 2.0f);
    float y = (v.y + 1) * (height / 2.0f);
    return std::make_tuple(x, y);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
        return 1;
    }

    Model model(argv[1]);
    TGAImage framebuffer(width, height, TGAImage::RGB);
    std::cout << "# Vertices: " << model.nvertices() << std::endl;
    std::cout << "# Faces: " << model.nfaces() << std::endl;

    for (int i = 0; i < model.nfaces(); i++) {
        auto [ax, ay] = viewport_transform(model.vertices(i, 0));
        auto [bx, by] = viewport_transform(model.vertices(i, 1));
        auto [cx, cy] = viewport_transform(model.vertices(i, 2));
        line(ax, ay, bx, by, framebuffer, red);
        line(bx, by, cx, cy, framebuffer, red);
        line(cx, cy, ax, ay, framebuffer, red);
    }

    for (int i = 0; i < model.nvertices(); i++) {
        auto [x, y] = viewport_transform(model.vertices(i));
        framebuffer.set(x, y, white);
    }

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
