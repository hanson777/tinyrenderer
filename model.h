#include "geometry.h"
#include "tgaimage.h"
#include <string>
#include <vector>

class Model {
  private:
    std::vector<vec4> m_vertices;
    std::vector<int> m_faces;
    std::vector<vec4> m_normals;
    std::vector<int> m_normIndices;
    std::vector<vec2> m_uvs;
    std::vector<int> m_uvIndices;
    TGAImage normalmap{};
    TGAImage diffusemap{};
    TGAImage specularmap{};

  public:
    Model(std::string filepath);
    size_t nverts() const;
    size_t nfaces() const;
    vec4 vert(const int i) const;
    vec4 vert(const int iface, const int nvert) const;
    vec4 normal(const vec2& uv) const;
    vec4 normal(const int iface, const int nvert) const;
    vec2 uv(const int iface, const int nvert) const;
	const TGAImage& diffuse() const;
	const TGAImage& specular() const;
};
