#include "model.h"
#include "tgaimage.h"
#include <iostream>
#include <string>
#include <utility>

constexpr int width = 128;
constexpr int height = 128;

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

void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage& framebuffer, TGAColor color) {
    line(ax, ay, bx, by, framebuffer, color);
    line(bx, by, cx, cy, framebuffer, color);
    line(cx, cy, ax, ay, framebuffer, color);
}

void fill_triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage& framebuffer, TGAColor color) {
    if (ay > by) {
        std::swap(ay, by);
        std::swap(ax, bx);
    }
    if (by > cy) {
        std::swap(by, cy);
        std::swap(bx, cx);
    }
    triangle(ax, ay, bx, by, cx, cy, framebuffer, color);
    int j = 0;
    for (int i = by; i >= ay; i--) {
        line(bx, i, ax, ay - j, framebuffer, color);
        j++;
    }
}

std::tuple<float, float> viewport_transform(vec3 v) {
    float x = (v.x + 1) * (width / 2.0f);
    float y = (v.y + 1) * (height / 2.0f);
    return std::make_tuple(x, y);
}

void draw_model(Model& model, TGAImage& framebuffer) {
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
}

int main(int argc, char** argv) {
    // if (argc != 2) {
    //     std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
    //     return 1;
    // }

    // Model model(argv[1]);
    // TGAImage framebuffer(width, height, TGAImage::RGB);
    // draw_model(model, framebuffer);

    TGAImage framebuffer(width, height, TGAImage::RGB);

    fill_triangle(7, 45, 35, 100, 45, 60, framebuffer, red);
    fill_triangle(120, 35, 90, 5, 45, 110, framebuffer, white);
    fill_triangle(115, 83, 80, 90, 85, 120, framebuffer, green);
    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
