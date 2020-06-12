#include "Geometry/Bones.h"
#include <glm/gtx/transform.hpp>

void Bones::LoadModel(const aiScene* scene)
{
    m_scene = scene;
    m_root_node_transform = to_glm(scene->mRootNode->mTransformation.Inverse());
}

void Bones::ProcessMesh(const aiMesh* mesh, IMesh& cur_mesh)
{
    std::vector<std::vector<BoneInfo>> per_vertex_bone_info(cur_mesh.positions.size());
    for (uint32_t i = 0; i < mesh->mNumBones; ++i)
    {
        uint32_t bone_index = 0;
        std::string bone_name(mesh->mBones[i]->mName.data);
        if (bone_mapping.find(bone_name) == bone_mapping.end())
        {
            bone_index = static_cast<uint32_t>(bone_mapping.size());
            bone_mapping[bone_name] = bone_index;
            if (bone_index >= bone_offset.size())
            {
                bone_offset.resize(bone_index + 1);
                bone.resize(bone_index + 1);
            }
            bone_offset[bone_index] = to_glm(mesh->mBones[i]->mOffsetMatrix);
        }
        else
        {
            bone_index = bone_mapping[bone_name];
        }

        for (uint32_t j = 0; j < mesh->mBones[i]->mNumWeights; ++j)
        {
            uint32_t vertex_id = mesh->mBones[i]->mWeights[j].mVertexId;
            float weight = mesh->mBones[i]->mWeights[j].mWeight;
            per_vertex_bone_info[vertex_id].push_back({ bone_index, weight });
        }
    }

    cur_mesh.bones_offset.resize(per_vertex_bone_info.size());
    cur_mesh.bones_count.resize(per_vertex_bone_info.size());
    for (uint32_t vertex_id = 0; vertex_id < per_vertex_bone_info.size(); ++vertex_id)
    {
        cur_mesh.bones_offset[vertex_id] = static_cast<uint32_t>(bone_info.size());
        cur_mesh.bones_count[vertex_id] = static_cast<uint32_t>(per_vertex_bone_info[vertex_id].size());
        std::copy(per_vertex_bone_info[vertex_id].begin(), per_vertex_bone_info[vertex_id].end(), back_inserter(bone_info));
    }
}

std::shared_ptr<Resource> Bones::GetBonesInfo(Device& device, CommandListBox& command_list)
{
    if (!bones_info_srv)
        bones_info_srv = device.CreateBuffer(BindFlag::kShaderResource, static_cast<uint32_t>(bone_info.size() * sizeof(BoneInfo)));
    if (!bone_info.empty())
        command_list.UpdateSubresource(bones_info_srv, 0, bone_info.data(), 0, 0);
    return bones_info_srv;
}

std::shared_ptr<Resource> Bones::GetBone(Device& device, CommandListBox& command_list)
{
    if (!bone_srv)
        bone_srv = device.CreateBuffer(BindFlag::kShaderResource, static_cast<uint32_t>(bone.size() * sizeof(glm::mat4)));
    if (!bone.empty())
        command_list.UpdateSubresource(bone_srv, 0, bone.data(), 0, 0);
    return bone_srv;
}

bool Bones::UpdateAnimation(float time_in_seconds)
{
    if (!m_scene)
        return false;
    if (!m_scene->mAnimations)
        return false;

    float ticks_per_second = (float)(m_scene->mAnimations[0]->mTicksPerSecond != 0 ? m_scene->mAnimations[0]->mTicksPerSecond : 25.0f);
    float time_in_ticks = time_in_seconds* ticks_per_second;
    float animation_time = fmod(time_in_ticks, (float)m_scene->mAnimations[0]->mDuration);

    glm::mat4 identity(1.0);
    ReadNodeHeirarchy(animation_time, m_scene->mRootNode, identity);

    return true;
}

glm::mat4 Bones::to_glm(const aiMatrix4x4& mat)
{
    return glm::mat4(
        mat.a1, mat.b1, mat.c1, mat.d1,
        mat.a2, mat.b2, mat.c2, mat.d2,
        mat.a3, mat.b3, mat.c3, mat.d3,
        mat.a4, mat.b4, mat.c4, mat.d4);
}

glm::mat3 Bones::to_glm(const aiMatrix3x3& mat)
{
    return glm::mat3(
        mat.a1, mat.b1, mat.c1,
        mat.a2, mat.b2, mat.c2,
        mat.a3, mat.b3, mat.c3);
}

void Bones::ReadNodeHeirarchy(float animation_time, const aiNode* node, const glm::mat4& parent_transform)
{
    const aiAnimation* animation = m_scene->mAnimations[0];
    std::string node_name(node->mName.data);
    const aiNodeAnim* node_anim = FindNodeAnim(animation, node_name);
    glm::mat4 node_transformation(to_glm(node->mTransformation));

    if (node_anim)
    {
        aiVector3D scaling = CalcInterpolatedScaling(animation_time, node_anim);
        glm::mat4 scaling_mat = glm::scale(glm::vec3(scaling.x, scaling.y, scaling.z));

        aiQuaternion rotation = CalcInterpolatedRotation(animation_time, node_anim);
        glm::mat4 rotatio_mat = to_glm(rotation.GetMatrix());

        aiVector3D translation = CalcInterpolatedPosition(animation_time, node_anim);
        glm::mat4 translation_mat = glm::translate(glm::vec3(translation.x, translation.y, translation.z));

        node_transformation = translation_mat* rotatio_mat* scaling_mat;
    }

    glm::mat4 current_transform = parent_transform* node_transformation;

    if (bone_mapping.find(node_name) != bone_mapping.end())
    {
        uint32_t BoneIndex = bone_mapping[node_name];
        bone[BoneIndex] = glm::transpose(m_root_node_transform* current_transform* bone_offset[BoneIndex]);
    }

    for (uint32_t i = 0; i < node->mNumChildren; ++i)
    {
        ReadNodeHeirarchy(animation_time, node->mChildren[i], current_transform);
    }
}

aiVector3D Bones::CalcInterpolatedScaling(float animation_time, const aiNodeAnim* node_anim)
{
    if (node_anim->mNumScalingKeys == 1)
        return node_anim->mScalingKeys[0].mValue;
    uint32_t ScalingIndex = FindScaling(animation_time, node_anim);
    if (ScalingIndex == -1)
        return {};
    uint32_t NextScalingIndex = (ScalingIndex + 1);
    assert(NextScalingIndex < node_anim->mNumScalingKeys);
    float DeltaTime = (float)(node_anim->mScalingKeys[NextScalingIndex].mTime - node_anim->mScalingKeys[ScalingIndex].mTime);
    float Factor = (animation_time - (float)node_anim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
    assert(Factor >= 0.0f && Factor <= 1.0f);
    const aiVector3D& Start = node_anim->mScalingKeys[ScalingIndex].mValue;
    const aiVector3D& End = node_anim->mScalingKeys[NextScalingIndex].mValue;
    aiVector3D Delta = End - Start;
    return Start + Factor* Delta;
}

aiQuaternion Bones::CalcInterpolatedRotation(float animation_time, const aiNodeAnim* node_anim)
{
    if (node_anim->mNumRotationKeys == 1)
        return node_anim->mRotationKeys[0].mValue;
    uint32_t RotationIndex = FindRotation(animation_time, node_anim);;
    if (RotationIndex == -1)
        return {};
    uint32_t NextRotationIndex = RotationIndex + 1;
    float DeltaTime = (float)(node_anim->mRotationKeys[NextRotationIndex].mTime - node_anim->mRotationKeys[RotationIndex].mTime);
    float Factor = (animation_time - (float)node_anim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
    assert(Factor >= 0.0f && Factor <= 1.0f);
    const aiQuaternion& StartRotationQ = node_anim->mRotationKeys[RotationIndex].mValue;
    const aiQuaternion& EndRotationQ = node_anim->mRotationKeys[NextRotationIndex].mValue;
    aiQuaternion res;
    aiQuaternion::Interpolate(res, StartRotationQ, EndRotationQ, Factor);
    return res.Normalize();
}

aiVector3D Bones::CalcInterpolatedPosition(float animation_time, const aiNodeAnim* node_anim)
{
    if (node_anim->mNumPositionKeys == 1)
        return node_anim->mPositionKeys[0].mValue;
    uint32_t PositionIndex = FindPosition(animation_time, node_anim);
    if (PositionIndex == -1)
        return {};
    uint32_t NextPositionIndex = PositionIndex + 1;
    float DeltaTime = (float)(node_anim->mPositionKeys[NextPositionIndex].mTime - node_anim->mPositionKeys[PositionIndex].mTime);
    float Factor = (animation_time - (float)node_anim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
    assert(Factor >= 0.0f && Factor <= 1.0f);
    const aiVector3D& Start = node_anim->mPositionKeys[PositionIndex].mValue;
    const aiVector3D& End = node_anim->mPositionKeys[NextPositionIndex].mValue;
    aiVector3D Delta = End - Start;
    return Start + Factor* Delta;
}

const aiNodeAnim* Bones::FindNodeAnim(const aiAnimation* animation, const std::string& node_name)
{
    for (uint32_t i = 0; i < animation->mNumChannels; ++i)
    {
        const aiNodeAnim* node_anim = animation->mChannels[i];
        if (std::string(node_anim->mNodeName.data) == node_name)
            return node_anim;
    }
    return nullptr;
}

uint32_t Bones::FindPosition(float animation_time, const aiNodeAnim* node_anim)
{
    for (uint32_t i = 0; i + 1 < node_anim->mNumPositionKeys; ++i)
    {
        if (animation_time < (float)node_anim->mPositionKeys[i + 1].mTime)
        {
            return i;
        }
    }
    assert(false);
    return -1;
}

uint32_t Bones::FindRotation(float animation_time, const aiNodeAnim* node_anim)
{
    for (uint32_t i = 0; i + 1 < node_anim->mNumRotationKeys; ++i)
    {
        if (animation_time < (float)node_anim->mRotationKeys[i + 1].mTime)
            return i;
    }
    assert(false);
    return -1;
}

uint32_t Bones::FindScaling(float animation_time, const aiNodeAnim* node_anim)
{
    for (uint32_t i = 0; i + 1 < node_anim->mNumScalingKeys; ++i)
    {
        if (animation_time < (float)node_anim->mScalingKeys[i + 1].mTime)
            return i;
    }
    assert(false);
    return -1;
}
