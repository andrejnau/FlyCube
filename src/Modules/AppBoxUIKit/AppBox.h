#pragma once

#include <memory>

struct AppSize {
    constexpr AppSize(uint32_t width, uint32_t height)
        : m_width(width)
        , m_height(height)
    {
    }

    constexpr uint32_t width() const
    {
        return m_width;
    }

    constexpr uint32_t height() const
    {
        return m_height;
    }

private:
    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

class AppRenderer {
public:
    virtual ~AppRenderer() = default;
    virtual void Init(const AppSize& app_size, void* window) = 0;
    virtual void Resize(const AppSize& app_size, void* window) {}
    virtual void Render() = 0;
};

class AppBox {
public:
    static AppBox& GetInstance();
    AppRenderer& GetRenderer();
    int Run(std::unique_ptr<AppRenderer> renderer, int argc, char* argv[]);

private:
    std::unique_ptr<AppRenderer> m_renderer;
};
