#pragma once
#include "ray_cast.h"
#include "renderer.h"
#include "renderer_types.h"
#include <array>
#include <d3d11.h>
#include "engine_math.h"

constexpr unsigned int MaxDebugLines = 1024;
enum DebugColors { Red, Green, Blue, Pink, Max };

constexpr std::array<glm::vec4, DebugColors::Max> colors = 
{
    glm::vec4(1.0, 0, 0, 1.0),
    glm::vec4(0, 1.0, 0, 1.0),
    glm::vec4(0, 0, 1.0, 1.0),
    glm::vec4(0.95, 0.003, 1.0, 1.0)
};

// keeps track of where a font is located in the font atlas
struct FontLocation 
{
    f32 start_x, start_y, end_x, end_y;
};

struct RendererFontCache 
{
    Texture2D font_atlas; // atlas to put all the fonts in
};

// allow to draw shapes for debugging help
struct DebugRenderSystem 
{

    u64 n_lines = 0;
    DebugLine lines[MaxDebugLines];
    GfxShaderProgram line_shader;

    //void init(GfxState* renderer);
    //void debug_line_vec4(XMVECTOR a, XMVECTOR b, XMFLOAT4 color);
    //void update_line_vec4(u32 i, XMVECTOR a, XMVECTOR b, XMFLOAT4 color);
    //void update_line_float3(u32 i, XMFLOAT3 a, XMFLOAT3 b, XMFLOAT4 color);
    //void debug_ray(Ray ray);
    //void clear_debug_lines();
    //void debug_text(std::string text,
    //                std::string font_family,
    //                f32 height,
    //                DebugColors color);

    // update GPU buffers
    void update(GfxState* renderer);
    void draw(GfxState* renderer);

    // static DebugRenderSystem &instance();

    ComPtr<ID3D11Buffer> vertex_buffer;
    ComPtr<ID3D11Buffer> index_buffer; // maybe unused
    ComPtr<ID3D11InputLayout> vertex_layout;
    ComPtr<ID3D11VertexShader> line_vertex_shader;
    ComPtr<ID3D11PixelShader> line_pixel_shader;
};

void debug_system_init(GfxState* gfx);
//void debug_line(glm::vec4 to, glm::vec4 from, glm::vec4 color);
void debug_line(glm::vec3 to, glm::vec3 from, glm::vec4 color);
void debug_ray(Ray* ray, glm::vec4 color, f32 length = 200.f);
void debug_text(const char* text, u32 length);

void debug_system_begin_frame();
void debug_system_end_frame();
