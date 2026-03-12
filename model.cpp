#include "model.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

Model::Model(std::string filepath) {
    std::ifstream in;
    in.open(filepath, std::ifstream::in);
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
            vec3 v;
            for (int i : { 0, 1, 2 }) {
                iss >> v[i];
            }
            m_vertices.push_back(v);
        } else if (!line.compare(0, 2, "f ")) {
            iss >> trash;
            int index;
            std::string ignored;
            for (int i = 0; i < 3; i++) {
                iss >> index;
                m_faces.push_back(--index);
                iss >> trash;
                iss >> index;
                iss >> trash;
                iss >> index;
                m_normIndices.push_back(--index);
            }
        } else if (!line.compare(0, 3, "vn ")) {
            iss >> trash >> trash;
            vec3 n;
            for (int i : {0,1,2}) iss >> n[i];
            m_normals.push_back(n);
        }
    }
}

size_t Model::nvertices() const { return m_vertices.size(); }
size_t Model::nfaces() const { return m_faces.size() / 3; }

vec3 Model::vertices(const int i) const { return m_vertices[i]; }
vec3 Model::vertices(const int ithFace, const int nthVertex) const {
    return m_vertices[m_faces[ithFace * 3 + nthVertex]];
}

vec3 Model::normals(const int i) const { return m_normals[i]; }
vec3 Model::normals(const int ithNormal, const int nthVertex) const {
    return m_normals[m_normIndices[ithNormal * 3 + nthVertex]];
}
