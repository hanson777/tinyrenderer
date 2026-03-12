#include "model.h"
#include "tgaimage.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

Model::Model(std::string filename) {
    std::ifstream in;
    in.open(filename, std::ifstream::in);
    if (in.fail()) {
        std::cerr << "error opening file" << std::endl;
        exit(1);
    }
    std::string line;
    char trash;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        if (!line.compare(0, 2, "v ")) {
            iss >> trash;
            vec4 v{ 0,0,0,1 };
            for (int i : { 0, 1, 2 }) {
                iss >> v[i];
            }
            m_vertices.push_back(v);
        } else if (!line.compare(0, 2, "f ")) {
            iss >> trash;
            int index{};
            for (int i = 0; i < 3; i++) {
                iss >> index;
                m_faces.push_back(--index);
                iss >> trash >> index;
                m_uvIndices.push_back(--index);
                iss >> trash >> index;
                m_normIndices.push_back(--index);
            }
        } else if (!line.compare(0, 3, "vn ")) {
            iss >> trash >> trash;
            vec4 n;
            for (int i : {0,1,2}) iss >> n[i];
            m_normals.push_back(n);
        }
        else if (!line.compare(0, 3, "vt ")) {
            iss >> trash >> trash;
            vec2 uv; 
            iss >> uv[0];
            iss >> uv[1];
            m_uvs.push_back({uv.x, 1-uv.y});
        }
    }

	auto load_texture = [&filename](const std::string& suffix, TGAImage& img) {
		const size_t dot = filename.find_last_of(".");
		if (dot == std::string::npos) return;
		const std::string texfile = filename.substr(0, dot) + suffix;
		std::cerr << "texture file " << texfile << " loading " << (img.read_tga_file(texfile.c_str()) ? "ok" : "failed") << std::endl;
		};
    load_texture("_diffuse.tga", diffusemap);
    load_texture("_nm_tangent.tga", normalmap);
    load_texture("_spec.tga", specularmap);
}

size_t Model::nverts() const { return m_vertices.size(); }
size_t Model::nfaces() const { return m_faces.size() / 3; }

vec4 Model::vert(const int i) const { return m_vertices[i]; }
vec4 Model::vert(const int iface, const int nvert) const { return m_vertices[m_faces[iface * 3 + nvert]]; }

vec4 Model::normal(const vec2& uv) const {
	TGAColor c = normalmap.get(uv[0] * normalmap.width(), uv[1] * normalmap.height());
	return vec4{ (double)c[2],(double)c[1],(double)c[0],0 }*2. / 255. - vec4{ 1,1,1,0 }; // get the tex coords and remap [0,255] -> [-1,1]
}
vec4 Model::normal(const int iface, const int nvert) const { return m_normals[m_normIndices[iface * 3 + nvert]]; }

vec2 Model::uv(const int iface, const int nvert) const { return m_uvs[m_uvIndices[iface * 3 + nvert]]; }

const TGAImage& Model::diffuse() const { return diffusemap; }
const TGAImage& Model::specular() const { return specularmap; }
