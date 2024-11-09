#include "Model.hpp"

#include <algorithm>

using namespace model;

inline constexpr glm::vec3 X(1, 0, 0);
inline constexpr glm::vec3 Y(0, 1, 0);
inline constexpr glm::vec3 Z(0, 0, 1);

void Mesh::addPlane(
    const glm::vec3& pos,
    const glm::vec3& right,
    const glm::vec3& up,
    const glm::vec3& norm
) {
    vertices.push_back({pos-right-up, {0,0}, norm});
    vertices.push_back({pos+right-up, {1,0}, norm});
    vertices.push_back({pos+right+up, {1,1}, norm});

    vertices.push_back({pos-right-up, {0,0}, norm});
    vertices.push_back({pos+right+up, {1,1}, norm});
    vertices.push_back({pos-right+up, {0,1}, norm});
}

void Mesh::addPlane(
    const glm::vec3& pos,
    const glm::vec3& right,
    const glm::vec3& up,
    const glm::vec3& norm,
    const UVRegion& uv
) {
    vertices.push_back({pos-right-up, {uv.u1, uv.v1}, norm});
    vertices.push_back({pos+right-up, {uv.u2, uv.v1}, norm});
    vertices.push_back({pos+right+up, {uv.u2, uv.v2}, norm});

    vertices.push_back({pos-right-up, {uv.u1, uv.v1}, norm});
    vertices.push_back({pos+right+up, {uv.u2, uv.v2}, norm});
    vertices.push_back({pos-right+up, {uv.u1, uv.v2}, norm});
}

void Mesh::addBox(const glm::vec3& pos, const glm::vec3& size) {
    addPlane(pos+Z*size, X*size, Y*size, Z);
    addPlane(pos-Z*size, -X*size, Y*size, -Z);

    addPlane(pos+Y*size, X*size, -Z*size, Y);
    addPlane(pos-Y*size, X*size, Z*size, -Y);

    addPlane(pos+X*size, -Z*size, Y*size, X);
    addPlane(pos-X*size, Z*size, Y*size, -X);
}

void Mesh::addBox(
    const glm::vec3& pos, const glm::vec3& size, const UVRegion (&uvs)[6]
) {
    addPlane(pos+Z*size, X*size, Y*size, Z, uvs[0]);
    addPlane(pos-Z*size, -X*size, Y*size, -Z, uvs[1]);

    addPlane(pos+Y*size, X*size, -Z*size, Y, uvs[2]);
    addPlane(pos-Y*size, X*size, Z*size, -Y, uvs[3]);

    addPlane(pos+X*size, -Z*size, Y*size, X, uvs[4]);
    addPlane(pos-X*size, Z*size, Y*size, -X, uvs[5]);
}

void Mesh::scale(const glm::vec3& size) {
    for (auto& vertex : vertices) {
        vertex.coord *= size;
    }
}

void Model::clean() {
    meshes.erase(
        std::remove_if(meshes.begin(), meshes.end(), 
        [](const Mesh& mesh){
            return mesh.vertices.empty();
        }),
        meshes.end()
    );
}
