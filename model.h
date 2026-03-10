#include "geometry.h"
#include <string>
#include <vector>

class Model {
  private:
    std::vector<vec3> m_vertices;
    std::vector<int> m_faces;

  public:
    Model(std::string filepath);
    size_t nvertices() const;
    size_t nfaces() const;
    vec3 vertices(const int i) const;
    vec3 vertices(const int i, const int currentVertex) const;
};
