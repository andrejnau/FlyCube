#include "SkinningPass.h"

SkinningPass::SkinningPass(Context& context, const Input& input, int width, int height)
    : m_context(context)
    , m_input(input)
    , m_program(context)
{
}

void SkinningPass::OnUpdate()
{
}

void SkinningPass::OnRender()
{
    m_context->UseProgram(m_program);
    m_context->Attach(m_program.cs.cbv.cb, m_program.cs.cbuffer.cb);

    for (auto& model : m_input.scene_list)
    {
        if (!model.bones.UpdateAnimation(glfwGetTime()))
            continue;

        std::shared_ptr<Resource> bones_info_srv = model.bones.GetBonesInfo(m_context);
        std::shared_ptr<Resource> bone_srv = model.bones.GetBone(m_context);

        m_context->Attach(m_program.cs.srv.index_buffer, model.ia.indices.GetBuffer());

        m_context->Attach(m_program.cs.srv.bone_info, bones_info_srv);
        m_context->Attach(m_program.cs.srv.gBones, bone_srv);
        m_context->Attach(m_program.cs.srv.bones_offset, model.ia.bones_offset.GetBuffer());
        m_context->Attach(m_program.cs.srv.bones_count, model.ia.bones_count.GetBuffer());

        m_context->Attach(m_program.cs.srv.in_position, model.ia.positions.GetBuffer());
        m_context->Attach(m_program.cs.srv.in_normal, model.ia.normals.GetBuffer());
        m_context->Attach(m_program.cs.srv.in_tangent, model.ia.tangents.GetBuffer());

        m_context->Attach(m_program.cs.uav.out_position, model.ia.positions.GetDynamicBuffer());
        m_context->Attach(m_program.cs.uav.out_normal, model.ia.normals.GetDynamicBuffer());
        m_context->Attach(m_program.cs.uav.out_tangent, model.ia.tangents.GetDynamicBuffer());

        for (auto& range : model.ia.ranges)
        {
            m_program.cs.cbuffer.cb.IndexCount = range.index_count;
            m_program.cs.cbuffer.cb.StartIndexLocation = range.start_index_location;
            m_program.cs.cbuffer.cb.BaseVertexLocation = range.base_vertex_location;
            m_context->Dispatch((range.index_count + 256 - 1) / 256, 1, 1);
        }
    }
}

void SkinningPass::OnModifySponzaSettings(const SponzaSettings& settings)
{
    m_settings = settings;
}
