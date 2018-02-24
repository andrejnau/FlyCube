#pragma once

#include <Texture/TextureInfo.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <string>
#include <algorithm>
#include <utility>
#include <vector>
#include <cstdint>
#include <map>

class DX11Context;

struct IMesh
{
    struct Material
    {
        glm::vec3 amb = glm::vec3(0.0, 0.0, 0.0);
        glm::vec3 dif = glm::vec3(1.0, 1.0, 1.0);
        glm::vec3 spec = glm::vec3(1.0, 1.0, 1.0);
        float shininess = 32.0;
        std::string name;
    } material;

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
    std::vector<glm::vec3> tangents;
    std::vector<glm::vec4> colors;
    std::vector<uint32_t> indices;
    std::vector<uint32_t> bones_offset;
    std::vector<uint32_t> bones_count;
    std::vector<TextureInfo> textures;
};

class Bones
{
public:
    struct BoneInfo
    {
        uint32_t bone_id;
        float bone_weight;
    };

    std::vector<BoneInfo> bone_info;
    std::vector<glm::mat4> bone;

    void LoadModel(const aiScene* scene)
    {
        m_scene = scene;
        m_root_node_transform = to_glm(scene->mRootNode->mTransformation.Inverse());
    }

    void ProcessMesh(const aiMesh* mesh, IMesh& cur_mesh)
    {
        std::vector<std::vector<BoneInfo>> per_vertex_bone_info(cur_mesh.positions.size());
        for (uint32_t i = 0; i < mesh->mNumBones; ++i)
        {
            uint32_t bone_index = 0;
            std::string bone_name(mesh->mBones[i]->mName.data);
            if (bone_mapping.find(bone_name) == bone_mapping.end())
            {
                bone_index = bone_mapping.size();
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
            cur_mesh.bones_offset[vertex_id] = bone_info.size();
            cur_mesh.bones_count[vertex_id] = per_vertex_bone_info[vertex_id].size();
            std::copy(per_vertex_bone_info[vertex_id].begin(), per_vertex_bone_info[vertex_id].end(), back_inserter(bone_info));
        }
    }

    void UpdateAnimation(float time_in_seconds)
    {
        if (!m_scene)
            return;
        if (!m_scene->mAnimations)
            return;

        float ticks_per_second = (float)(m_scene->mAnimations[0]->mTicksPerSecond != 0 ? m_scene->mAnimations[0]->mTicksPerSecond : 25.0f);
        float time_in_ticks = time_in_seconds * ticks_per_second;
        float animation_time = fmod(time_in_ticks, (float)m_scene->mAnimations[0]->mDuration);

        glm::mat4 identity(1.0);
        ReadNodeHeirarchy(animation_time, m_scene->mRootNode, identity);
    }

private:
    std::map<std::string, uint32_t> bone_mapping;
    std::vector<glm::mat4> bone_offset;
    const aiScene* m_scene = nullptr;
    glm::mat4 m_root_node_transform;
    
    inline glm::mat4 to_glm(const aiMatrix4x4& mat)
    {
        return glm::mat4(
            mat.a1, mat.b1, mat.c1, mat.d1,
            mat.a2, mat.b2, mat.c2, mat.d2,
            mat.a3, mat.b3, mat.c3, mat.d3,
            mat.a4, mat.b4, mat.c4, mat.d4);
    }

    inline glm::mat3 to_glm(const aiMatrix3x3& mat)
    {
        return glm::mat3(
            mat.a1, mat.b1, mat.c1,
            mat.a2, mat.b2, mat.c2,
            mat.a3, mat.b3, mat.c3);
    }

    void ReadNodeHeirarchy(float animation_time, const aiNode* node, const glm::mat4& parent_transform)
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

            node_transformation = translation_mat * rotatio_mat * scaling_mat;
        }

        glm::mat4 current_transform = parent_transform * node_transformation;

        if (bone_mapping.find(node_name) != bone_mapping.end())
        {
            uint32_t BoneIndex = bone_mapping[node_name];
            bone[BoneIndex] = glm::transpose(m_root_node_transform * current_transform * bone_offset[BoneIndex]);
        }

        for (uint32_t i = 0; i < node->mNumChildren; ++i)
        {
            ReadNodeHeirarchy(animation_time, node->mChildren[i], current_transform);
        }
    }

    aiVector3D CalcInterpolatedScaling(float animation_time, const aiNodeAnim* node_anim)
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
        return Start + Factor * Delta;
    }

    aiQuaternion CalcInterpolatedRotation(float animation_time, const aiNodeAnim* node_anim)
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

    aiVector3D CalcInterpolatedPosition(float animation_time, const aiNodeAnim* node_anim)
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
        return Start + Factor * Delta;
    }

    const aiNodeAnim* FindNodeAnim(const aiAnimation* animation, const std::string& node_name)
    {
        for (uint32_t i = 0; i < animation->mNumChannels; ++i)
        {
            const aiNodeAnim* node_anim = animation->mChannels[i];
            if (std::string(node_anim->mNodeName.data) == node_name)
                return node_anim;
        }
        return nullptr;
    }

    uint32_t FindPosition(float animation_time, const aiNodeAnim* node_anim)
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

    uint32_t FindRotation(float animation_time, const aiNodeAnim* node_anim)
    {
        for (uint32_t i = 0; i + 1 < node_anim->mNumRotationKeys; ++i)
        {
            if (animation_time < (float)node_anim->mRotationKeys[i + 1].mTime)
                return i;
        }
        assert(false);
        return -1;
    }

    uint32_t FindScaling(float animation_time, const aiNodeAnim* node_anim)
    {
        for (uint32_t i = 0; i + 1 < node_anim->mNumScalingKeys; ++i)
        {
            if (animation_time < (float)node_anim->mScalingKeys[i + 1].mTime)
                return i;
        }
        assert(false);
        return -1;
    }
};

struct BoundBox
{
    BoundBox()
    {
        x_min = y_min = z_min = std::numeric_limits<float>::max();
        x_max = y_max = z_max = std::numeric_limits<float>::min();
    }

    float x_min, y_min, z_min;
    float x_max, y_max, z_max;
};

struct IModel
{
    virtual void AddMesh(const IMesh& mesh) = 0;
    virtual BoundBox& GetBoundBox() = 0;
    virtual const BoundBox& GetBoundBox() const = 0;
    virtual Bones& GetBones() = 0;
};

class ModelLoader
{
public:
    ModelLoader(const std::string& file, aiPostProcessSteps flags, IModel& meshes);

private:
    std::string SplitFilename(const std::string& str);
    void LoadModel(aiPostProcessSteps flags);
    void ProcessNode(aiNode* node, const aiScene* scene);
    void ProcessMesh(aiMesh* mesh, const aiScene* scene);
    void FindSimilarTextures(std::vector<TextureInfo>& textures);
    void LoadMaterialTextures(aiMaterial* mat, aiTextureType type, std::vector<TextureInfo>& textures);

private:
    std::string m_path;
    std::string m_directory;
    Assimp::Importer m_import;
    IModel& m_model;
    Bones& m_bones;
};

template<typename Mesh>
struct Model : IModel
{
    Model(DX11Context& context, const std::string& file, uint32_t flags = ~0)
        : m_context(context)
        , m_model_loader(file, (aiPostProcessSteps)flags, *this)
    {
    }

    virtual void AddMesh(const IMesh& mesh) override
    {
        meshes.emplace_back(m_context, mesh);
    }

    virtual BoundBox& GetBoundBox() override
    {
        return bound_box;
    }

    virtual const BoundBox& GetBoundBox() const override
    {
        return bound_box;
    }

    virtual Bones& GetBones() override
    {
        return bones;
    }

    BoundBox bound_box;
    Bones bones;
    std::vector<Mesh> meshes;

private:
    DX11Context& m_context;
    ModelLoader m_model_loader;
};

struct ModelSize
{
    ModelSize(const IModel& model)
    {
        const BoundBox& bound_box = model.GetBoundBox();
        z_width = (bound_box.z_max - bound_box.z_min);
        y_width = (bound_box.y_max - bound_box.y_min);
        x_width = (bound_box.x_max - bound_box.x_min);
        model_width = std::max({ z_width, y_width, x_width });
        scale = 1.0 / model_width;
        model_width *= scale;
        offset_x = (bound_box.x_max + bound_box.x_min) / 2.0f;
        offset_y = (bound_box.y_max + bound_box.y_min) / 2.0f;
        offset_z = (bound_box.z_max + bound_box.z_min) / 2.0f;
    }

    float z_width;
    float y_width;
    float x_width;
    float model_width;
    float scale;
    float offset_x;
    float offset_y;
    float offset_z;
};

template<typename Mesh>
struct ListItem
{
    ListItem(DX11Context& context, const std::string& file)
        : model(context, file)
    {
    }

    Model<Mesh> model;
    glm::mat4 matrix;
};

template<typename Mesh>
using SceneList = std::vector<ListItem<Mesh>>;
