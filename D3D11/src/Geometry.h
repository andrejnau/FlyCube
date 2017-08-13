#pragma once

#include <Windows.h>
#include <d3d11.h>

#include <vector>
#include <fstream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <SimpleMath.h>
#include <wrl.h>

using namespace Microsoft::WRL;
using namespace DirectX::SimpleMath;

struct CommandHelper;

struct Mesh
{
    struct Vertex
    {
        Vector3 position;
        Vector3 normal;
        Vector2 texCoords;
        Vector3 tangent;
        Vector3 bitangent;
    };

    struct Texture
    {
        aiTextureType type;
        std::string path;
        uint32_t offset;

        ComPtr<ID3D11ShaderResourceView> textureRV;
    };

    struct Material
    {
        Vector3 amb = Vector3(0.0, 0.0, 0.0);
        Vector3 dif = Vector3(1.0, 1.0, 1.0);
        Vector3 spec = Vector3(1.0, 1.0, 1.0);
        float shininess = 32.0;
        aiString name;
    } material;

    ComPtr<ID3D11Buffer> vertBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<Texture> textures;
    void setupMesh(ComPtr<ID3D11Device>& d3d11Device, ComPtr<ID3D11DeviceContext>& d3d11DevCon);
};

class Model
{
public:
    Model(const std::string & file);

    std::string m_path;
    std::string m_directory;
    std::vector<Mesh> meshes;

    struct BoundBox
    {
        BoundBox();
        float x_min, y_min, z_min;
        float x_max, y_max, z_max;
    } boundBox;

private:
    std::string splitFilename(const std::string& str);
    void loadModel();

    void processNode(aiNode* node, const aiScene* scene);

    Vector3 aiColor4DToVec3(const aiColor4D& x);

    Mesh processMesh(aiMesh* mesh, const aiScene* scene);

    void loadMaterialTextures(Mesh &retMeh, aiMaterial* mat, aiTextureType type);
};
