#include "geometry.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

class Model {
  public:
    std::vector<vec3> m_vertices;
    std::vector<int> m_faces;

  public:
    Model(std::string filepath);
    int nvertices() const;
    int nfaces() const;
    vec3 vertices(const int i) const;
    vec3 vertices(const int i, const int currentVertex) const;
};
