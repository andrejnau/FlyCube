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
    m_context.UseProgram(m_program);

    for (auto& model : m_input.scene_list)
    {
        if (!model.bones.UpdateAnimation(glfwGetTime()))
            continue;

        Resource::Ptr bones_info_srv = model.bones.GetBonesInfo(m_context);
        Resource::Ptr bone_srv = model.bones.GetBone(m_context);

        m_program.cs.srv.index_buffer.Attach(model.ia.indices.GetBuffer());

        m_program.cs.srv.bone_info.Attach(bones_info_srv);
        m_program.cs.srv.gBones.Attach(bone_srv);
        m_program.cs.srv.bones_offset.Attach(model.ia.bones_offset.GetBuffer());
        m_program.cs.srv.bones_count.Attach(model.ia.bones_count.GetBuffer());

        m_program.cs.srv.in_position.Attach(model.ia.positions.GetBuffer());
        m_program.cs.srv.in_normal.Attach(model.ia.normals.GetBuffer());
        m_program.cs.srv.in_tangent.Attach(model.ia.tangents.GetBuffer());

        m_program.cs.uav.out_position.Attach(model.ia.positions.GetDynamicBuffer());
        m_program.cs.uav.out_normal.Attach(model.ia.normals.GetDynamicBuffer());
        m_program.cs.uav.out_tangent.Attach(model.ia.tangents.GetDynamicBuffer());

        for (auto& range : model.ia.ranges)
        {
            m_program.cs.cbuffer.cbv.IndexCount = range.index_count;
            m_program.cs.cbuffer.cbv.StartIndexLocation = range.start_index_location;
            m_program.cs.cbuffer.cbv.BaseVertexLocation = range.base_vertex_location;
            m_context.Dispatch((range.index_count + 256 - 1) / 256, 1, 1);
        }
    }
}

void SkinningPass::OnModifySettings(const Settings& settings)
{
    m_settings = settings;
}
