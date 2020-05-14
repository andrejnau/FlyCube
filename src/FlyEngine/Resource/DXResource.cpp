#include "Resource/DXResource.h"
#include <View/DXView.h>
#include <Device/DXDevice.h>
#include <Utilities/FileUtility.h>

DXResource::DXResource(DXDevice& device)
    : m_device(device)
{
}

gli::format DXResource::GetFormat() const
{
    return m_format;
}

void DXResource::SetName(const std::string& name)
{
    default_res->SetName(utf8_to_wstring(name).c_str());
}

ComPtr<ID3D12Resource>& DXResource::GetUploadResource(size_t subresource)
{
    if (subresource >= m_upload_res.size())
        m_upload_res.resize(subresource + 1);
    return m_upload_res[subresource];
}
