#include "Resource/DX12Resource.h"
#include "Context/DX12Context.h"

DX12Resource::DX12Resource(DX12Context& context)
    : m_context(context)
{
}

DX12Resource::~DX12Resource()
{
    m_context.QueryOnDelete(default_res.Get());
}

ComPtr<ID3D12Resource>& DX12Resource::GetUploadResource(size_t subresource)
{
    if (subresource >= m_upload_res.size())
        m_upload_res.resize(subresource + 1);
    return m_upload_res[subresource];
}

void DX12Resource::SetName(const std::string & name)
{
    default_res->SetName(utf8_to_wstring(name).c_str());
}
