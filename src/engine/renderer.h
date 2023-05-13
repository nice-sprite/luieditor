#ifndef GFX_HELPERS_H
#define GFX_HELPERS_H

#include "../defines.h"
#include "files.h"
#include "gpu_resources.h"
#include "logging.h"
#include "renderer_types.h"
#include <d3d11.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include "engine_math.h"

using namespace Microsoft::WRL;

template<typename T>
void safe_release(T** com_object) 
{
    if(*com_object != nullptr) 
    {
        (*com_object)->Release();
        *com_object = nullptr;
    }
}
 
struct RenderTarget 
{
    ID3D11Texture2D* render_target;  // TODO: dont know if keeping this is needed?
    ID3D11RenderTargetView* render_target_view;
    ID3D11ShaderResourceView* srv;
    glm::vec2 min;
    glm::vec2 max;
    DXGI_FORMAT format;
};

struct RenderTargetSettings 
{
    u32 width;
    u32 height;
    DXGI_FORMAT format;
};

RenderTarget render_target_create(struct GfxState* gfx, RenderTargetSettings settings);
b8 render_target_resize(RenderTarget* target, struct GfxState* gfx, f32 new_width, f32 new_height); 

struct GfxShaderProgram 
{ 
    ID3D11VertexShader* vs;
    ID3D11PixelShader* ps;
    ID3D11InputLayout* input_layout;
    char* source_path; 
    std::string shader_id;
    b8 precompiled;
}; 

// TODO remove fs::path and std::string from this type of stuff,
// TODO change all "create" type operations from taking 
// a path to taking in the bytes of the file that would be loaded
// this way the functions become simpler and file loading can be done however we choose, 
// especially noticable when breaking stuff out into threads
struct GfxShaderCreateParams 
{
    u32 source_code_length;
    char* source_code;

    u32 source_code_path_length;
    char* source_code_path; // for hot reload

    u32 shader_name_length; 
    char* shader_name;

    u32 input_element_count;
    D3D11_INPUT_ELEMENT_DESC *input_element_desc; 

    b8 hotreload_enable;
    b8 precompiled;
}; 

GfxShaderProgram shader_program_create(struct GfxState* gfx, GfxShaderCreateParams shader_params);
void shader_bind(GfxState* gfx, GfxShaderProgram* shader);
void shader_destroy(GfxShaderProgram* program);

// holds both the swapchains backbuffer render_texture_view and depth/stencil buffer view 
struct Backbuffer 
{ 
    ID3D11Texture2D*        depth_stencil_texture;
    ID3D11RenderTargetView* render_target_view;
    ID3D11DepthStencilView* depth_stencil_view;
};

struct GfxDeferredRenderTargetResizeEvent
{
    RenderTarget* target;
    glm::vec2 new_size; 
};

struct GfxMappedMemory
{ 
    void* mem; 
    u32 row_pitch;
    u32 depth_pitch; 
}; 



/* Keep track of D3D device, context, swapchain, 
 * and common state objects like render views etc 
 */
struct GfxState 
{
    ID3D11DeviceContext*    context;
    ID3D11Device*           device;
    IDXGISwapChain1*        swapchain; 
    Backbuffer              current_backbuffer;
    ID3D11BlendState*       alpha_blend;
    ID3D11RasterizerState*  rasterizer_state;
    ID3D11SamplerState*     sampler_nearest_wrapuvs;
    IDXGIDevice2*           dxgi_device;
    IDXGIFactory2*          dxgi_factory;
    IDXGIAdapter*           dxgi_adapter;
    GfxShaderProgram        shader_fullscreen_quad;
    GlobalShaderConstants   global_shader_constants; // assert that this is 16 byte aligned for better copy perf
    ID3D11Buffer*           shader_constants_buffer; // shader constants that anything can write to

    D3D11_VIEWPORT viewport{}; 
    u32 stack_index; 
    RenderTarget *render_texture_stack[4]; 

    u32 resize_event_count;
    GfxDeferredRenderTargetResizeEvent resize_events[4]; // 

    glm::vec4 swapchain_size;
};


GfxState* gfx_start(void* window, i32 width, i32 height);
void gfx_begin_frame(GfxState* gfx);
void gfx_end_frame(GfxState* gfx);
void gfx_shutdown(GfxState* gfx);
void gfx_present(GfxState* gfx, u8 vsync);
void gfx_push_render_texture(GfxState* gfx, RenderTarget* target);
void gfx_pop_render_texture(GfxState* gfx);
void gfx_deferred_target_resize(GfxState* gfx, RenderTarget* target, glm::vec2 new_size);
void gfx_set_topology(GfxState* gfx, D3D11_PRIMITIVE_TOPOLOGY topo);
void gfx_draw_indexed(GfxState* gfx, u32 index_count);
ID3D11Buffer* shader_buffer_create(GfxState* gfx, u32 size_in_bytes_16_byte_aligned, void* initial_data = 0);
GfxMappedMemory gfx_map_global_constants_for_write(GfxState* gfx);
void gfx_unmap_global_constants(GfxState* gfx);
void gfx_set_constant_buffer(GfxState* state, ID3D11Buffer* buffer);

/* Backbuffer/Swapchain management */
void backbuffer_clear(GfxState* gfx, Backbuffer* bb, glm::vec4 clear_color);
void backbuffer_bind(GfxState* gfx, Backbuffer* bb);
void backbuffer_resize(GfxState* gfx, Backbuffer* bb, i32 width, i32 height, b8 main_window_minimized);
Backbuffer backbuffer_create(GfxState* gfx, f32 width, f32 height, b8 use_depth);
void backbuffer_reset(GfxState* gfx); // TODO probably delete this as the only time we do it is in the resize event, so resize event should just handle this 

/* Shader management */
void pixel_shader_create(char* src_path, ID3D11PixelShader **out_shader);
void pixel_shader_bind(ID3D11PixelShader *shader);

//  TEXTURE management
Texture2D gfx_texture_create(GfxState* gfx, TextureParams const &params, Texture2D &out_texture);
void gfx_texture_update_subregion(Texture2D& texture, u32 subresource, D3D11_BOX* region, void* src_data, u32 src_pitch, u32 src_depth_pitch = 0);
void gfx_texture_bind(GfxState* gfx, Texture2D const* texture);

void vertex_buffer_create(GfxState* gfx, ID3D11Buffer **out_buffer, u32 size_bytes);
void vertex_buffer_bind(GfxState* gfx, ID3D11Buffer **buffers, u32 buff_count, u32 stride, u32 offset);
GfxMappedMemory vertex_buffer_map_for_write(GfxState* gfx, ID3D11Buffer* buffer);
void vertex_buffer_unmap(GfxState* gfx, ID3D11Buffer* buffer);
void index_buffer_unmap(GfxState* gfx, ID3D11Buffer* buffer);

void index_buffer_create(GfxState* gfx, ID3D11Buffer **out_buffer, u32 index_count);
void index_buffer_bind(GfxState* gfx, ID3D11Buffer *buffer);
GfxMappedMemory index_buffer_map_for_write(GfxState* gfx, ID3D11Buffer* buffer);

//class _Renderer {
//
//public:
//
//  Renderer(HWND hwnd, i32 width, i32 height);
//
//  ~Renderer();
//
//  Renderer(const Renderer &Gfx) = delete;
//
//  Renderer &operator=(const Renderer &) = delete;
//
//  /* Backbuffer/Swapchain management */
//  void backbuffer_clear(glm::vec4 clear_color);
//  void backbuffer_bind();
//  void backbuffer_resize(int width, int height, bool minimized);
//  void backbuffer_view_create();
//  void backbuffer_view_reset();
//  f32 backbuffer_aspect_ratio();
//
//  void present();
//
//
//  /* Shader management */
//  void pixel_shader_create(fs::path src_path, ID3D11PixelShader **out_shader);
//  void pixel_shader_bind(ID3D11PixelShader *shader);
//
//
//
//  void vertex_shader_bind(ID3D11VertexShader *shader);
//
//  /* Buffer management */
//  void vertex_buffer_create(ID3D11Buffer **out_buffer, u32 size_bytes);
//  void vertex_buffer_bind(ID3D11Buffer **buffers, u32 buff_count, u32 stride, u32 offset);
//
//  void index_buffer_create(ID3D11Buffer **out_buffer, u32 num_indices);
//  void index_buffer_bind(ID3D11Buffer *buffer); 
//
//  void set_topology(D3D11_PRIMITIVE_TOPOLOGY topo);
//
//  /* Texture management */
//  
//  /* creates a texture
//   * returns true if success, false otherwise
//   */
//  bool texture_create(GfxState* gfx, TextureParams const &params, Texture2D &out_texture);
//  void texture_update_subregion(Texture2D& texture, u32 subresource, D3D11_BOX* region, void* src_data, u32 src_pitch, u32 src_depth_pitch = 0);
//  void texture_bind(Texture2D * texture);
//
//
//  /* Render Texture management */
//  void render_texture_bind();
//  void render_texture_clear(glm::vec4 clear_color);
//  void render_texture_create();
//  void render_texture_resize(f32 w, f32 h);
//
//  /* implement a stack for push/popping render textures(?) */
//  void render_texture_push(RenderTarget* rtv);
//  void render_texture_pop();
//
//  /* submit draw calls */
//  void draw_indexed(u32 num_indices);
//  void draw(u32 vert_count);
//
//private:
//
//  /* Creates dx11 device, swapchain, and objects for rendering to `hwnd`
//   * swapchain dimensions will be `width` x `height`
//   * */
//  bool d3d11_init(HWND hwnd, i32 width, i32 height);
//
//public:
//
//  f32 width, height;
//
//  ComPtr<ID3D11Device5> device;
//  ComPtr<ID3D11DeviceContext4> context;
//
//  PerSceneConsts scene_consts;
//  ComPtr<ID3D11Buffer> scene_constant_buffer;
//
//  void draw_fullscreen_quad();
//
//  void set_viewport(ViewportRegion viewport);
//
//
//private:
//  /* TODO GfxState */
//  ComPtr<IDXGISwapChain4> swapChain;
//
//  // D3D
//  D3D11_VIEWPORT viewport{};
//  ComPtr<ID3D11BlendState> alphaBlendState;
//  ComPtr<ID3D11RasterizerState2> rasterizerState;
//  ComPtr<ID3D11RenderTargetView> rtv;
//  ComPtr<IDXGIFactory7> dxgiFactory;
//  ComPtr<ID3D11SamplerState> gridSS;
//  ComPtr<ID3D11Texture2D> depth_stencil_texture;
//  ComPtr<ID3D11DepthStencilView> depth_stencil_view;
//
//  ComPtr<ID3D11PixelShader> ps_fullscreen_quad;
//  ComPtr<ID3D11VertexShader> vs_fullscreen_quad;
//  ComPtr<ID3D11InputLayout> fullscreen_quad_il;
//
//
//  DirectX::XMFLOAT4 clear_color;
//  HWND hwnd;
//};

#endif // !GFX_HELPERS_H
