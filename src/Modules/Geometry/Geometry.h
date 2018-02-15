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

class Context;

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

    std::map<std::string, uint32_t> bone_mapping;
    std::vector<BoneInfo> bone_info_flat;

    struct BoneMatrix
    {
        glm::mat4 offsset;
        glm::mat4 FinalTransformation;
    };

    std::vector<BoneMatrix> bone_matrix;

    glm::mat4 aiMatrix4x4ToMat4(const aiMatrix4x4& mat)
    {
        glm::mat4 res;
        res[0][0] = mat.a1; res[1][0] = mat.a2;
        res[2][0] = mat.a3; res[3][0] = mat.a4;
        res[0][1] = mat.b1; res[1][1] = mat.b2;
        res[2][1] = mat.b3; res[3][1] = mat.b4;
        res[0][2] = mat.c1; res[1][2] = mat.c2;
        res[2][2] = mat.c3; res[3][2] = mat.c4;
        res[0][3] = mat.d1; res[1][3] = mat.d2;
        res[2][3] = mat.d3; res[3][3] = mat.d4;
        return res;
    }

    glm::mat3 aiMatrix3x3ToMat3(const aiMatrix3x3& mat)
    {
        glm::mat4 res;
        res[0][0] = mat.a1; res[0][1] = mat.b1; res[0][2] = mat.c1;
        res[1][0] = mat.a2; res[1][1] = mat.b2; res[1][2] = mat.c2;
        res[2][0] = mat.a3; res[2][1] = mat.b3; res[2][2] = mat.c3; 
        return res;
    }

    void BoneTransform(float TimeInSeconds, std::vector<glm::mat4>& Transforms)
    {
        if (!m_pScene->mAnimations)
            return;
        glm::mat4 Identity(1.0);

        float TicksPerSecond = (float)(m_pScene->mAnimations[0]->mTicksPerSecond != 0 ? m_pScene->mAnimations[0]->mTicksPerSecond : 25.0f);
        float TimeInTicks = TimeInSeconds * TicksPerSecond;
        float AnimationTime = fmod(TimeInTicks, (float)m_pScene->mAnimations[0]->mDuration);

        ReadNodeHeirarchy(AnimationTime, m_pScene, m_pScene->mRootNode, Identity);

        Transforms.resize(bone_mapping.size());

        for (uint32_t i = 0; i < Transforms.size(); i++)
        {
            Transforms[i] = glm::transpose(bone_matrix[i].FinalTransformation);
        }
    }

    void ReadNodeHeirarchy(float AnimationTime, const aiScene* m_pScene, const aiNode* pNode, const glm::mat4& ParentTransform)
    {
        std::string NodeName(pNode->mName.data);

        const aiAnimation* pAnimation = m_pScene->mAnimations[0];
        glm::mat4 NodeTransformation(aiMatrix4x4ToMat4(pNode->mTransformation));

        const aiNodeAnim* pNodeAnim = FindNodeAnim(pAnimation, NodeName);

        if (pNodeAnim)
        {
            // Interpolate scaling and generate scaling transformation matrix
            aiVector3D Scaling;
            CalcInterpolatedScaling(Scaling, AnimationTime, pNodeAnim);
            glm::mat4 ScalingM;
            ScalingM = glm::scale(glm::vec3(Scaling.x, Scaling.y, Scaling.z));

            // Interpolate rotation and generate rotation transformation matrix
            aiQuaternion RotationQ;
            CalcInterpolatedRotation(RotationQ, AnimationTime, pNodeAnim);
            glm::mat4 RotationM = aiMatrix3x3ToMat3(RotationQ.GetMatrix());

            // Interpolate translation and generate translation transformation matrix
            aiVector3D Translation;
            CalcInterpolatedPosition(Translation, AnimationTime, pNodeAnim);
            glm::mat4 TranslationM;
            TranslationM = glm::translate(glm::vec3(Translation.x, Translation.y, Translation.z));

            // Combine the above transformations
            NodeTransformation = TranslationM * RotationM * ScalingM;
        }

        glm::mat4 GlobalTransformation = ParentTransform * NodeTransformation;

        if (bone_mapping.find(NodeName) != bone_mapping.end())
        {
            uint32_t BoneIndex = bone_mapping[NodeName];
            bone_matrix[BoneIndex].FinalTransformation = aiMatrix4x4ToMat4(m_pScene->mRootNode->mTransformation.Inverse()) * GlobalTransformation * bone_matrix[BoneIndex].offsset;
        }

        for (uint32_t i = 0; i < pNode->mNumChildren; i++)
        {
            ReadNodeHeirarchy(AnimationTime, m_pScene, pNode->mChildren[i], GlobalTransformation);
        }
    }


    void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
    {
        if (pNodeAnim->mNumScalingKeys == 1)
        {
            Out = pNodeAnim->mScalingKeys[0].mValue;
            return;
        }

        uint32_t ScalingIndex = FindScaling(AnimationTime, pNodeAnim);
        uint32_t NextScalingIndex = (ScalingIndex + 1);
        assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
        float DeltaTime = (float)(pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime);
        float Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
        assert(Factor >= 0.0f && Factor <= 1.0f);
        const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
        const aiVector3D& End = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
        aiVector3D Delta = End - Start;
        Out = Start + Factor * Delta;
    }

    void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
    {
        // we need at least two values to interpolate...
        if (pNodeAnim->mNumRotationKeys == 1)
        {
            Out = pNodeAnim->mRotationKeys[0].mValue;
            return;
        }

        uint32_t RotationIndex = FindRotation(AnimationTime, pNodeAnim);
        uint32_t NextRotationIndex = (RotationIndex + 1);
        assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
        float DeltaTime = (float)(pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime);
        float Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
        assert(Factor >= 0.0f && Factor <= 1.0f);
        const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
        const aiQuaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;
        aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);
        Out = Out.Normalize();
    }

    void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
    {
        if (pNodeAnim->mNumPositionKeys == 1)
        {
            Out = pNodeAnim->mPositionKeys[0].mValue;
            return;
        }

        uint32_t PositionIndex = FindPosition(AnimationTime, pNodeAnim);
        uint32_t NextPositionIndex = (PositionIndex + 1);
        assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
        float DeltaTime = (float)(pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime);
        float Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
        assert(Factor >= 0.0f && Factor <= 1.0f);
        const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
        const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
        aiVector3D Delta = End - Start;
        Out = Start + Factor * Delta;
    }

    const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, const std::string& NodeName)
    {
        for (uint32_t i = 0; i < pAnimation->mNumChannels; ++i)
        {
            const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];
            if (std::string(pNodeAnim->mNodeName.data) == NodeName)
                return pNodeAnim;
        }
        return nullptr;
    }

    uint32_t FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim)
    {
        for (uint32_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; ++i)
        {
            if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime)
                return i;
        }
        assert(0);
        return 0;
    }

    uint32_t FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
    {
        assert(pNodeAnim->mNumRotationKeys > 0);
        for (uint32_t i = 0; i < pNodeAnim->mNumRotationKeys - 1; ++i)
        {
            if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime)
                return i;
        }
        assert(0);
        return 0;
    }

    uint32_t FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim)
    {
        assert(pNodeAnim->mNumScalingKeys > 0);
        for (uint32_t i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++)
        {
            if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime)
                return i;
        }
        assert(0);
        return 0;
    }

    const aiScene* m_pScene = nullptr;
    void LoadBones(aiMesh* mesh, const aiScene* pScene, IMesh& cur_mesh)
    {
        if (!m_pScene)
            m_pScene = pScene;
        std::vector<std::vector<BoneInfo>> bone_info(cur_mesh.positions.size());
        if (!mesh->mNumBones)
        {
            int b = 0;
        }
        for (uint32_t i = 0; i < mesh->mNumBones; ++i)
        {
            std::string bone_name(mesh->mBones[i]->mName.data);
            uint32_t bone_index = 0;
            if (bone_mapping.find(bone_name) == bone_mapping.end())
            {
                bone_index = bone_mapping.size();
                bone_mapping[bone_name] = bone_index;
                if (bone_index >= bone_matrix.size())
                    bone_matrix.resize(bone_index + 1);
                bone_matrix[bone_index].offsset = aiMatrix4x4ToMat4(mesh->mBones[i]->mOffsetMatrix);
            }
            else
            {
                bone_index = bone_mapping[bone_name];
            }

            for (uint32_t j = 0; j < mesh->mBones[i]->mNumWeights; ++j)
            {
                uint32_t vertex_id = mesh->mBones[i]->mWeights[j].mVertexId;
                float weight = mesh->mBones[i]->mWeights[j].mWeight;
                bone_info[vertex_id].push_back({ bone_index, weight });
            }
        }

        cur_mesh.bones_offset.resize(bone_info.size());
        cur_mesh.bones_count.resize(bone_info.size());
        static size_t maxq = 0;
        for (uint32_t vertex_id = 0; vertex_id < bone_info.size(); ++vertex_id)
        {
            cur_mesh.bones_offset[vertex_id] = bone_info_flat.size();
            cur_mesh.bones_count[vertex_id] = bone_info[vertex_id].size();
            maxq = std::max(maxq, bone_info[vertex_id].size());
            std::copy(bone_info[vertex_id].begin(), bone_info[vertex_id].end(), back_inserter(bone_info_flat));
        }
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
    Model(Context& context, const std::string& file, uint32_t flags = ~0)
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
    Context& m_context;
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
    ListItem(Context& context, const std::string& file)
        : model(context, file)
    {
    }

    Model<Mesh> model;
    glm::mat4 matrix;
};

template<typename Mesh>
using SceneList = std::vector<ListItem<Mesh>>;
