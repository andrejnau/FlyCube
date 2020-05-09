#include "Resource/DX12Resource.h"
#include "Context/DX12Context.h"
#include <Utilities/FileUtility.h>

DX12Resource::DX12Resource(Destroyer& destroyer)
    : m_destroyer(destroyer)
{
}

DX12Resource::~DX12Resource()
{
    m_destroyer.get().QueryOnDelete(default_res.Get());
    for (auto& upload : m_upload_res)
    {
        m_destroyer.get().QueryOnDelete(upload.Get());
    }
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
