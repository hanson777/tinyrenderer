#include "tgaimage.h"
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>

constexpr TGAColor white = { 255, 255, 255, 255 }; // attention, BGRA order
constexpr TGAColor green = { 0, 255, 0, 255 };
constexpr TGAColor red = { 0, 0, 255, 255 };
constexpr TGAColor blue = { 255, 128, 64, 255 };
constexpr TGAColor yellow = { 0, 200, 255, 255 };

struct Vertex {
    float x;
    float y;
    float z;
};

struct Face {
    uint32_t x;
    uint32_t y;
    uint32_t z;
};

struct Model {
    std::vector<Vertex> vertices;
    std::vector<Face> faces;
};

void loadOBJ(std::string filepath, Model& model) {
    std::ifstream infile(filepath);
    std::string line;
    if (!infile.is_open()) {
        std::cerr << "error opening file" << std::endl;
    }
    exit(1);
    while (std::getline(infile, line)) {
        std::cout << line << '\n';
        if (line[0] == 'v') {
            line = line.substr(1);
            std::stringstream stream(line);
            float value;
            uint32_t i = 0;
            Vertex vertex;
            while (stream >> value) {
                std::cout << "Value: " << value << '\n';
                if (i == 0) vertex.x = value;
                if (i == 1) vertex.y = value;
                if (i == 2) vertex.z = value;
            }
            std::cout << "X: " << vertex.x << '\n';
            std::cout << "Y: " << vertex.y << '\n';
            std::cout << "Z: " << vertex.z << '\n';
            model.vertices.push_back(vertex);
        }
        else if (line[0] == 'f') {
			line = line.substr(1);
			std::stringstream stream(line);
			uint32_t value;
			uint32_t i = 0;
            Face face;
			while (stream >> value) {
				if (i == 0) face.x = value;
				if (i == 1) face.y = value;
				if (i == 2) face.z = value;
			}
			model.faces.push_back(face);
        }
    }
}

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

int main(int argc, char** argv) {
    constexpr int width = 64;
    constexpr int height = 64;
    TGAImage framebuffer(width, height, TGAImage::RGB);

    Model model;
    loadOBJ("obj/diablo3_pose/diablo3_pose.obj", model);


    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
