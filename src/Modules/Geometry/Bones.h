#pragma once

#include "Geometry/Mesh.h"
#include <Resource/Resource.h>
#include <assimp/scene.h>
#include <vector>
#include <glm/glm.hpp>

class Bones
{
public:
    void LoadModel(const aiScene* scene);
    void ProcessMesh(const aiMesh* mesh, IMesh& cur_mesh);
    std::shared_ptr<Resource> GetBonesInfo();
    std::shared_ptr<Resource> GetBone();
    bool HasAnimation();
    void UpdateAnimation(Device& device, CommandListBox& command_list, float time_in_seconds);

private:
    void ReadNodeHeirarchy(float animation_time, const aiNode* node, const glm::mat4& parent_transform);
    aiVector3D CalcInterpolatedScaling(float animation_time, const aiNodeAnim* node_anim);
    aiQuaternion CalcInterpolatedRotation(float animation_time, const aiNodeAnim* node_anim);
    aiVector3D CalcInterpolatedPosition(float animation_time, const aiNodeAnim* node_anim);
    const aiNodeAnim* FindNodeAnim(const aiAnimation* animation, const std::string& node_name);
    uint32_t FindPosition(float animation_time, const aiNodeAnim* node_anim);
    uint32_t FindRotation(float animation_time, const aiNodeAnim* node_anim);
    uint32_t FindScaling(float animation_time, const aiNodeAnim* node_anim);

    glm::mat4 to_glm(const aiMatrix4x4& mat);
    glm::mat3 to_glm(const aiMatrix3x3& mat);

    struct BoneInfo
    {
        uint32_t bone_id;
        float bone_weight;
    };

    std::vector<BoneInfo> bone_info;
    std::vector<glm::mat4> bone;

    std::shared_ptr<Resource> bones_info_srv;
    std::shared_ptr<Resource> bone_srv;
    std::map<std::string, uint32_t> bone_mapping;
    std::vector<glm::mat4> bone_offset;
    const aiScene* m_scene = nullptr;
    glm::mat4 m_root_node_transform;
};
