#pragma once

#include <glLoadGen/gl.h>
#include "Context/Resource.h"

class GLResource : public Resource
{
public:
    using Ptr = std::shared_ptr<GLResource>;

    GLuint texture = 0;
    GLuint buffer = 0;
    GLuint buffer_size = 0;

    struct Image
    {
        GLsizei width;
        GLsizei height;
        size_t level_count;
        GLenum int_format;
        GLenum format;
        GLenum type;
        bool is_compressed = false;
    } image;

    enum class Type
    {
        kUnknown,
        kBuffer,
        kImage,
        kSampler
    };

    SamplerDesc sampler;

    Type res_type = Type::kUnknown;

    virtual void SetName(const std::string& name) override
    {
    }
};
