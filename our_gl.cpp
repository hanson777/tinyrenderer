#include "our_gl.h"
#include <algorithm>
#include <array>

mat<4, 4> ModelView, Viewport, Perspective;
std::vector<double> zbuffer;

void lookat(const vec3 &eye, const vec3 &center, const vec3& up) {
    const vec3 n = normalized(eye - center);
    const vec3 l = normalized(cross(up, n));
    const vec3 m = normalized(cross(n, l));
    ModelView = mat<4, 4>{ {{l.x,l.y,l.z,0}, {m.x,m.y,m.z,0}, {n.x,n.y,n.z,0}, {0,0,0,1}} } *
        mat<4, 4>{{{1, 0, 0, -center.x}, { 0,1,0,-center.y }, { 0,0,1,-center.z }, { 0,0,0,1 }}};
}

void init_perspective(const double f) {
    Perspective = { {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0, -1 / f,1}} };
}

void init_viewport(const int x, const int y, const int w, const int h) {
    Viewport = { {{w / 2., 0, 0, x + w / 2.}, {0, h / 2., 0, y + h / 2.}, {0,0,1,0}, {0,0,0,1}} };
}

void init_zbuffer(const int width, const int height) {
    zbuffer = std::vector(width*height, -std::numeric_limits<double>::max());
}
void rasterize(const Triangle& clip, const IShader& shader, TGAImage& framebuffer) {
    const std::array<vec4, 3> ndc = { clip[0] / clip[0].w, clip[1] / clip[1].w, clip[2] / clip[2].w };
    std::array<vec2, 3> screen = { (Viewport * ndc[0]).xy(), (Viewport * ndc[1]).xy(), (Viewport * ndc[2]).xy() }; // screen coordinates

    const mat<3, 3> ABC = { { {screen[0].x, screen[0].y, 1.}, {screen[1].x, screen[1].y, 1.}, {screen[2].x, screen[2].y, 1.} } }; // det = 2x signed area
    if (ABC.det() < 1) return; // backface culling + discarding triangles that cover less than a pixel

    auto [bbmin_x, bbmax_x] = std::minmax({screen[0].x, screen[1].x, screen[2].x});
    auto [bbmin_y, bbmax_y] = std::minmax({screen[0].y, screen[1].y, screen[2].y });

    bbmin_x = std::max<double>(bbmin_x, 0);
    bbmin_y = std::max<double>(bbmin_y, 0);
    bbmax_x = std::min<double>(bbmax_x, framebuffer.width() - 1);
    bbmax_y = std::min<double>(bbmax_y, framebuffer.height() - 1);

    for (int x = bbmin_x; x <= bbmax_x; x++) {
        for (int y = bbmin_y; y <= bbmax_y; y++) {
            vec3 bc = ABC.invert_transpose() * vec3 { static_cast<double>(x), static_cast<double>(y), 1. }; // barycentric coordinates of {x,y} w.r.t the triangle
            if (bc.x < 0 || bc.y < 0 || bc.z < 0) continue;                                                 // negative barycentric coordinate -> the pixel is outside the triangle
            const double z = bc * vec3{ ndc[0].z, ndc[1].z, ndc[2].z };
            if (z <= zbuffer[x + y * framebuffer.width()]) continue;                                        // discard fragments that are too deep w.r.t the zbuffer
            vec3 bc_clip = { bc.x / clip[0].w, bc.y / clip[1].w, bc.z / clip[2].w };                       // perspective-correct interpolation
            bc_clip = bc_clip / (bc_clip.x + bc_clip.y + bc_clip.z);
            auto [discard, color] = shader.fragment(bc_clip);
            if (discard) continue;                                                                          // fragment shader can discard current fragment
            zbuffer[x + y * framebuffer.width()] = z;
            framebuffer.set(x, y, color);
        }
    }
}
