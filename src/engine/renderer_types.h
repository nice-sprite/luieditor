#pragma once
#include "../defines.h"
#include <array>
#include <d3d11_4.h>
#include <wrl/client.h>
#include <glm/glm.hpp>

struct VertexLayout 
{
    u32 size_in_bytes;
    u32 element_count;
    D3D11_INPUT_ELEMENT_DESC* element_desc;
};

struct VertexPosColorTexcoord 
{
    glm::vec3 pos;         // 3 * 4 = 12
    glm::vec4 color;      // 16 bytes
    glm::vec2 texcoord;    // 8 bytes
    // total size should = 12 + 16 = 28 + 8 = 32
    static VertexLayout layout()
    {
        static VertexLayout layout = VertexLayout
        {
            sizeof(VertexPosColorTexcoord),
            3,
            new D3D11_INPUT_ELEMENT_DESC[3]{
                {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0}, 
            }, 
        }; 
        return layout;
    }
};

struct VertexPosColor 
{
    glm::vec3 pos;
    glm::vec4 color;
    static VertexLayout layout();
};

struct DebugLine 
{
    glm::vec3 begin;
    glm::vec3 end;
    glm::vec4 color;
};

__declspec(align(16)) struct GlobalShaderConstants
{
    glm::mat4 modelViewProjection; // 64 bytes
    glm::vec4 timeTickDeltaFrame;  // 16 bytes
    glm::vec4 viewportSize;        // 8 bytes
};

struct ViewportRegion 
{
    f32 x, y, w, h;
};

// todo switch off comptr
struct Texture2D 
{
    ID3D11Texture2D* texture;
    ID3D11ShaderResourceView* srv;
    u32 width, height;
};

enum ResourceLimits 
{
    MaxTextureWidth = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION,
    MaxTextureHeight = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION,
    MaxConstantBufferElems = D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT,

};

typedef i32 TextureFormat;

enum TextureFormats : TextureFormat {
    RGBA_8 = DXGI_FORMAT_R8G8B8A8_UNORM,
};

struct TextureParams {
    u32 desired_width;
    u32 desired_height;
    TextureFormat format;
    D3D11_USAGE usage;
    D3D11_CPU_ACCESS_FLAG cpu_flags;
    void *initial_data;
};
