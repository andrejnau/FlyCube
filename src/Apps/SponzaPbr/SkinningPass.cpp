#include "SkinningPass.h"

SkinningPass::SkinningPass(Device& device, const Input& input)
    : m_device(device)
    , m_input(input)
    , m_program(device)
{
}

void SkinningPass::OnUpdate()
{
}

void SkinningPass::OnRender(CommandListBox& command_list)
{
    command_list.UseProgram(m_program);
    command_list.Attach(m_program.cs.cbv.cb, m_program.cs.cbuffer.cb);

    for (auto& model : m_input.scene_list)
    {
        if (!model.bones.UpdateAnimation(glfwGetTime()))
            continue;

        std::shared_ptr<Resource> bones_info_srv = model.bones.GetBonesInfo(m_device, command_list);
        std::shared_ptr<Resource> bone_srv = model.bones.GetBone(m_device, command_list);

        command_list.Attach(m_program.cs.srv.index_buffer, model.ia.indices.GetBuffer());

        command_list.Attach(m_program.cs.srv.bone_info, bones_info_srv);
        command_list.Attach(m_program.cs.srv.gBones, bone_srv);
        command_list.Attach(m_program.cs.srv.bones_offset, model.ia.bones_offset.GetBuffer());
        command_list.Attach(m_program.cs.srv.bones_count, model.ia.bones_count.GetBuffer());

        command_list.Attach(m_program.cs.srv.in_position, model.ia.positions.GetBuffer());
        command_list.Attach(m_program.cs.srv.in_normal, model.ia.normals.GetBuffer());
        command_list.Attach(m_program.cs.srv.in_tangent, model.ia.tangents.GetBuffer());

        command_list.Attach(m_program.cs.uav.out_position, model.ia.positions.GetDynamicBuffer());
        command_list.Attach(m_program.cs.uav.out_normal, model.ia.normals.GetDynamicBuffer());
        command_list.Attach(m_program.cs.uav.out_tangent, model.ia.tangents.GetDynamicBuffer());

        for (auto& range : model.ia.ranges)
        {
            m_program.cs.cbuffer.cb.IndexCount = range.index_count;
            m_program.cs.cbuffer.cb.StartIndexLocation = range.start_index_location;
            m_program.cs.cbuffer.cb.BaseVertexLocation = range.base_vertex_location;
            command_list.Dispatch((range.index_count + 256 - 1) / 256, 1, 1);
        }
    }
}

void SkinningPass::OnModifySponzaSettings(const SponzaSettings& settings)
{
    m_settings = settings;
}
