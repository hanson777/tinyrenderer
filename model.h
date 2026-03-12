#include "geometry.h"
#include <string>
#include <vector>

class Model {
  private:
    std::vector<vec3> m_vertices;
    std::vector<int> m_faces;
    std::vector<vec3> m_normals;
    std::vector<int> m_normIndices;

  public:
    Model(std::string filepath);
    size_t nvertices() const;
    size_t nfaces() const;
    vec3 vertices(const int i) const;
    vec3 vertices(const int ithFace, const int nthVertex) const;
    vec3 normals(const int i) const;
    vec3 normals(const int ithNormal, const int nthVertex) const;
};
