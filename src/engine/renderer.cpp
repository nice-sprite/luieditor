#include "./renderer.h"
#include "files.h"
#include "gpu_resources.h"
#include "logging.h"
#include "ray_cast.h"
#include <Tracy.hpp>
#include <d3d11.h>
#include <d3dcommon.h>
#include <dxgiformat.h>
#include <d3dcompiler.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include "win32_lib.h"
#include <fstream>
#include <iterator>

GfxState* gfx_start(void* window, i32 width, i32 height) 
{ 
    GfxState* gfx = new GfxState;
    gfx->stack_index = 0; 
    gfx->resize_event_count = 0;

    UINT createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG | D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };

    HRESULT res = D3D11CreateDevice(nullptr,
                                    D3D_DRIVER_TYPE_HARDWARE,
                                    nullptr,
                                    createDeviceFlags,
                                    featureLevels,
                                    _countof(featureLevels),
                                    D3D11_SDK_VERSION,
                                    &gfx->device,
                                    &featureLevel,
                                    &gfx->context);
    Q_ASSERT(SUCCEEDED(res));


    res = gfx->device->QueryInterface(__uuidof(IDXGIDevice2), (void**)&gfx->dxgi_device);
    Q_ASSERT(SUCCEEDED(res));

    res = gfx->dxgi_device->GetAdapter(&gfx->dxgi_adapter);
    Q_ASSERT(SUCCEEDED(res));

    res = gfx->dxgi_adapter->GetParent(__uuidof(IDXGIFactory2), (void**)&gfx->dxgi_factory);
    Q_ASSERT(SUCCEEDED(res));
    LOG_INFO("Got DXGI objects");


    LOG_INFO("creating swapchain...");

    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC1 sd = {0};
    sd.Width = (u32)width;
    sd.Height = (u32)height;
    sd.BufferCount = 2;
    sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    gfx->dxgi_factory->CreateSwapChainForHwnd(gfx->device,
                                              (HWND)window,
                                              &sd,
                                              nullptr,
                                              nullptr,
                                              &gfx->swapchain);

    LOG_INFO("creating blend states");
    D3D11_BLEND_DESC blend_alpha_blend{};
    blend_alpha_blend.RenderTarget[0].BlendEnable = true;
    blend_alpha_blend.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blend_alpha_blend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blend_alpha_blend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blend_alpha_blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blend_alpha_blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
    blend_alpha_blend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blend_alpha_blend.RenderTarget[0].RenderTargetWriteMask = 0x0f;

    // Setup blend state
    const float blend_factor[4] = {0.f, 0.f, 0.f, 0.f};
    gfx->device->CreateBlendState(&blend_alpha_blend, &gfx->alpha_blend);
    gfx->context->OMSetBlendState(gfx->alpha_blend, blend_factor, 0xffffffff);

    // setup raster state
    LOG_INFO("creating blend states");
    D3D11_RASTERIZER_DESC rasterDesc{};
    rasterDesc.AntialiasedLineEnable = true;
   // rasterDesc.CullMode = D3D11_CULL_BACK;
    rasterDesc.CullMode = D3D11_CULL_NONE;
    rasterDesc.DepthBias = 0;
    rasterDesc.DepthBiasClamp = 0.0f;
    // NOTE(nice_sprite): this is a lazy fix for a issue that came up when switching to GLM.
    // for some reason, the GLM matrices are causing the Z to be set to 1.0 in the vertex shader
    // when doing the MVP multiply. I think this has to do with inverted Z in GLM?
    rasterDesc.DepthClipEnable = false; // TODO setup state for debug lines that has this as false?
    rasterDesc.FillMode = D3D11_FILL_SOLID;
    rasterDesc.FrontCounterClockwise = false;
    rasterDesc.MultisampleEnable = true;
    rasterDesc.ScissorEnable = false; // TODO true?
    rasterDesc.SlopeScaledDepthBias = 0.0f;
    res = gfx->device->CreateRasterizerState(&rasterDesc, &gfx->rasterizer_state);
    Q_ASSERT(SUCCEEDED(res));
    gfx->context->RSSetState(gfx->rasterizer_state);

    gfx->viewport.Height = (float)height;
    gfx->viewport.Width = (float)width;
    gfx->viewport.MinDepth = 0.0f;
    gfx->viewport.MaxDepth = 1.0f;
    gfx->viewport.TopLeftX = 0.0f;
    gfx->viewport.TopLeftY = 0.0;
    gfx->context->RSSetViewports(1, &gfx->viewport);

    LOG_INFO("viewport config: {} {}", height, width);

    D3D11_SAMPLER_DESC gridSampler{};
    gridSampler.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    gridSampler.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    gridSampler.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    gridSampler.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    gridSampler.MipLODBias = 0;
    gridSampler.MaxAnisotropy = 1;
    gridSampler.ComparisonFunc = D3D11_COMPARISON_NEVER;
    gridSampler.BorderColor[0] = 0.f;
    gridSampler.BorderColor[1] = 0.f;
    gridSampler.BorderColor[2] = 0.f;
    gridSampler.BorderColor[3] = 0.f;
    gridSampler.MinLOD = -FLT_MAX;
    gridSampler.MaxLOD = FLT_MAX;
    gfx->device->CreateSamplerState(&gridSampler, &gfx->sampler_nearest_wrapuvs);
    gfx->context->PSSetSamplers(0, 1, &gfx->sampler_nearest_wrapuvs);

    gfx->current_backbuffer = backbuffer_create(gfx, width, height, true); // TODO: pass config param for use_depth?


    //gfx->global_shader_constants 
    gfx->shader_constants_buffer = shader_buffer_create(gfx, sizeof(GlobalShaderConstants)); 

    return(gfx);
}

void gfx_begin_frame(GfxState* gfx)
{
}

void gfx_end_frame(GfxState* gfx)
{ 
    for(i32 i = 0; i < gfx->resize_event_count; ++i)
    {

        b8 resize_result = render_target_resize(gfx->resize_events[i].target,
           gfx, 
           gfx->resize_events[i].new_size.x, 
           gfx->resize_events[i].new_size.y); 

        if(!resize_result)
        {
            LOG_FATAL("failed to resize render target!!");
            return; // immediately return because this is an unrecoverable state
        } 
    } 
    gfx->resize_event_count = 0; // this should be enough to "clear" resize events 
}

void gfx_present(GfxState* gfx, u8 vsync)
{
    gfx->swapchain->Present(vsync, 0);
}

void gfx_push_render_texture(GfxState* gfx, RenderTarget* target)
{
    static float clear_color[] = { 0, 0, 0, 1.0};
    if(gfx->stack_index < 3)
    {
        glm::vec2 w_h;
        D3D11_VIEWPORT viewport{};
        FLOAT2_SUBTRACT(w_h, target->max, target->min);
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;
        viewport.Width = w_h.x;
        viewport.Height = w_h.y;
        viewport.MinDepth = 0.0;
        viewport.MaxDepth = 1.0;

        gfx->render_texture_stack[gfx->stack_index] = target;

        // TODO: here, we probably do want a depth buffer bc the scene does care about element depth
        gfx->context->ClearRenderTargetView(target->render_target_view, clear_color);
        gfx->context->OMSetRenderTargets(1, &target->render_target_view, 0);
        gfx->context->RSSetViewports(1, &viewport);

        ++gfx->stack_index; 
    }
    else
    {
        LOG_WARNING("tried to push too many render targets!");
    }
}

void gfx_pop_render_texture(GfxState* gfx)
{
    if(gfx->stack_index == 0)
    {
        // bind the backbuffer
        backbuffer_bind(gfx, &gfx->current_backbuffer);
    }
    else
    {
        // bind the previous render target
        RenderTarget* t = gfx->render_texture_stack[gfx->stack_index-1];
        gfx->context->OMSetRenderTargets(1, &t->render_target_view, 0);
        --gfx->stack_index; 
    } 

}

void gfx_deferred_target_resize(GfxState* gfx, RenderTarget* target, glm::vec2 new_size)
{
    gfx->resize_events[gfx->resize_event_count] = GfxDeferredRenderTargetResizeEvent{target, new_size};
    ++gfx->resize_event_count;
}

void shader_report_shader_error(std::string shader_id, ID3D10Blob* error_blob) 
{
    if (error_blob) 
    {
        LOG_WARNING("while compiling {}: {}", shader_id, error_blob->GetBufferPointer());
    } 
    else 
    {
        LOG_WARNING("unknown shader error while compiling {}", shader_id);
    }
}

void gfx_set_topology(GfxState* gfx, D3D11_PRIMITIVE_TOPOLOGY topo)
{
    gfx->context->IASetPrimitiveTopology(topo); 
}

void gfx_draw_indexed(GfxState* gfx, u32 index_count)
{ 
    gfx->context->DrawIndexed(index_count, 0, 0);
}

GfxMappedMemory gfx_map_global_constants_for_write(GfxState* gfx) 
{
    GfxMappedMemory my_mapped{0};
    D3D11_MAPPED_SUBRESOURCE mapped{0}; 
    gfx->context->Map(gfx->shader_constants_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped); 
    my_mapped.mem = mapped.pData;
    my_mapped.row_pitch = mapped.RowPitch;
    my_mapped.depth_pitch = mapped.DepthPitch; 
    return my_mapped; 
}

void gfx_unmap_global_constants(GfxState* gfx) 
{
    gfx->context->Unmap(gfx->shader_constants_buffer, 0);
}

void gfx_set_constant_buffer(GfxState* gfx, ID3D11Buffer* buffer)
{ 
    gfx->context->VSSetConstantBuffers(0, 1, &buffer);
    gfx->context->PSSetConstantBuffers(0, 1, &buffer);
}

// To use this, cbuffer must be created with default usage type 
void gfx_update_global_shader_constants(GfxState* gfx)
{ 
    gfx->context->UpdateSubresource(gfx->shader_constants_buffer, 
        0, 
        0, 
        (void*)&gfx->global_shader_constants,
        0, 
        0);
}

ID3D11Buffer* shader_buffer_create(GfxState* gfx, u32 size_in_bytes_16_byte_aligned, void* initial_data)
{
    ID3D11Buffer* result_cbuffer = 0;
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = size_in_bytes_16_byte_aligned;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA init{};
    init.pSysMem = initial_data;
    init.SysMemPitch = 0;
    init.SysMemSlicePitch = 0;
    if(initial_data)
    {
        gfx->device->CreateBuffer(&desc, &init, &result_cbuffer); 
    }
    else
    {

        gfx->device->CreateBuffer(&desc, 0, &result_cbuffer); 
    }

    return result_cbuffer;
}

GfxShaderProgram shader_program_create(struct GfxState *gfx, GfxShaderCreateParams shader_params) 
{
    GfxShaderProgram out_prog{};
    HRESULT check;
    ID3D10Blob *error_blob;
    ID3D10Blob *vertex_blob;
    ID3D10Blob *pixel_blob;
    UINT d3dcompiler_flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG;

    // kind of forcing this right now, might allow other versions to be configured later
    const char *vs_shader_model = "vs_5_0";
    const char *ps_shader_model = "ps_5_0";

    const char *vertex_entrypoint = "vs_main";
    const char *pixel_entrypoint = "ps_main"; 

    if(shader_params.source_code)
    { 
        /* compile the vertex shader */
        check = D3DCompile(shader_params.source_code,
            shader_params.source_code_length, 
            shader_params.shader_name,
            nullptr, // defines
            nullptr, // includes
            vertex_entrypoint,
            vs_shader_model,
            d3dcompiler_flags,
            0,
            &vertex_blob,
            &error_blob);

        if (SUCCEEDED(check)) 
        {
            /* create the input layout for the vertex shader */
            check = gfx->device->CreateInputLayout(shader_params.input_element_desc,
                shader_params.input_element_count,
                vertex_blob->GetBufferPointer(),
                vertex_blob->GetBufferSize(),
                &out_prog.input_layout);

            if(SUCCEEDED(check))
            { 
                /* compile the pixel shader */
                check = D3DCompile(shader_params.source_code,
                    shader_params.source_code_length, 
                    shader_params.shader_name,
                    nullptr,
                    nullptr,
                    pixel_entrypoint,
                    ps_shader_model,
                    d3dcompiler_flags,
                    0,
                    &pixel_blob,
                    &error_blob);

                if(SUCCEEDED(check))
                {
                    gfx->device->CreateVertexShader(vertex_blob->GetBufferPointer(), vertex_blob->GetBufferSize(), 0, &out_prog.vs);
                    gfx->device->CreatePixelShader(pixel_blob->GetBufferPointer(), pixel_blob->GetBufferSize(), 0, &out_prog.ps);
                    return out_prog;

                }
            }
        } 
        else
        {
            LOG_WARNING("failed to compile vertex shader {}", shader_params.shader_name);
        }
    }
    else
    { 
        LOG_WARNING("did not supply source for shader {}", shader_params.shader_name);
    }

    // TODO PICK UP HERE
    return {};

}

void shader_bind(GfxState* gfx, GfxShaderProgram* shader)
{
    gfx->context->IASetInputLayout(shader->input_layout);
    gfx->context->PSSetShader(shader->ps, 0, 0);
    gfx->context->VSSetShader(shader->vs, 0, 0);
}

// TODO: do shaders need to be unbound from the pipeline before being destroyed?
void shader_destroy(GfxShaderProgram* program)
{
    LOG_INFO("destroying {}", program->shader_id);
    if(program->vs)
    {
        program->vs->Release();
    }

    if(program->ps)
    {
        program->ps->Release();
    }

    if(program->input_layout)
    {
        program->input_layout->Release(); 
    }
}


Backbuffer backbuffer_create(GfxState* gfx, f32 width, f32 height, b8 using_depth)
{
    ID3D11Texture2D* swapchain_texture = nullptr;
    Backbuffer new_backbuffer{0};
    gfx->swapchain->GetBuffer(0, IID_PPV_ARGS(&swapchain_texture));
    gfx->device->CreateRenderTargetView(swapchain_texture, 0, &new_backbuffer.render_target_view);
    swapchain_texture->Release();

    if(using_depth)
    { 
        D3D11_TEXTURE2D_DESC d;
        d.Width = (UINT)width;
        d.Height = (UINT)height;
        d.MipLevels = 1;
        d.ArraySize = 1;
        d.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        d.SampleDesc.Count = 1;
        d.SampleDesc.Quality = 0;
        d.Usage = D3D11_USAGE_DEFAULT;
        d.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        d.CPUAccessFlags = 0;
        d.MiscFlags = 0;

        gfx->device->CreateTexture2D(&d, NULL, &new_backbuffer.depth_stencil_texture);
        gfx->device->CreateDepthStencilView(new_backbuffer.depth_stencil_texture,
                                            NULL,
                                            &new_backbuffer.depth_stencil_view);
    }

    return new_backbuffer;
}

void backbuffer_bind(GfxState* gfx, Backbuffer* bb)
{
    gfx->context->OMSetRenderTargets(1, &bb->render_target_view, bb->depth_stencil_view);
}

void backbuffer_resize(GfxState* gfx, 
        Backbuffer* bb, 
        i32 new_width,
        i32 new_height,
        b8 minimized) 
{
    LOG_INFO("resizing swapchain old: [{}, {}] -> [{}, {}]",
            gfx->swapchain_size.x,
            gfx->swapchain_size.y,
            new_width,
            new_height);

    if (gfx->device && !minimized) 
    {
        b8 using_depth = false;
        if(bb->render_target_view)
        {
            bb->render_target_view->Release();
        }
        if(bb->depth_stencil_view) 
        {
            using_depth = true;
            bb->depth_stencil_view->Release(); 

        }

        gfx->swapchain->ResizeBuffers(0,
                new_width,
                new_height,
                DXGI_FORMAT_B8G8R8A8_UNORM,
                0);

        gfx->swapchain_size.x = new_width; 
        gfx->swapchain_size.y = new_width; 
        gfx->viewport.Width = new_width;
        gfx->viewport.Height = new_height;

        /*recreate the backbuffer views*/
        ID3D11Texture2D *pBackBuffer;
        gfx->swapchain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        gfx->device->CreateRenderTargetView(pBackBuffer, nullptr, &bb->render_target_view);
        pBackBuffer->Release();

        if(using_depth)
        {
            D3D11_TEXTURE2D_DESC d;
            d.Width = (UINT)new_width;
            d.Height = (UINT)new_height;
            d.MipLevels = 1;
            d.ArraySize = 1;
            d.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
            d.SampleDesc.Count = 1;
            d.SampleDesc.Quality = 0;
            d.Usage = D3D11_USAGE_DEFAULT;
            d.BindFlags = D3D11_BIND_DEPTH_STENCIL;
            d.CPUAccessFlags = 0;
            d.MiscFlags = 0;

            gfx->device->CreateTexture2D(&d, NULL, &bb->depth_stencil_texture);
            gfx->device->CreateDepthStencilView(bb->depth_stencil_texture,
                                                NULL,
                                                &bb->depth_stencil_view);

        } 
    }
    gfx->viewport.Width = new_width;
    gfx->viewport.Height = new_height;
    // TODO:  this is not really a solution - the imgui window can be bigger than the window size itself.
    // What _actually_ needs to happen is when we push a viewport, set RSSetViewports to the dimensions of the render target.
    gfx->context->RSSetViewports(1, &gfx->viewport);
}

void backbuffer_clear(GfxState* gfx, Backbuffer* bb, glm::vec4 clear_color) 
{
    if(bb->depth_stencil_view)
    { 
        gfx->context->ClearDepthStencilView(bb->depth_stencil_view,
            D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
            1.0f,
            0);
    }
    //context->RSSetViewports(1, &viewport);
    //context->OMSetRenderTargets(1, rtv.GetAddressOf(), depth_stencil_view.Get());
    // context->ClearDepthStencilView(depth_stencil_view.Get(),
    //                                D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
    //                                1.0f,
    //                                0);

    gfx->context->ClearRenderTargetView(bb->render_target_view, (float*)&clear_color);
}

// TEXTURE MANGEMENT

Texture2D gfx_texture_create(GfxState* gfx, TextureParams const &params, Texture2D &out_texture)
{
    LOG_FATAL("NOT IMPLEMENTED!");
    return Texture2D{};
}

void gfx_texture_update_subregion(Texture2D& texture,
        u32 subresource,
        D3D11_BOX* region,
        void* src_data,
        u32 src_pitch,
        u32 src_depth_pitch)
{
    LOG_FATAL("NOT IMPLEMENTED!");
}
void gfx_texture_bind(GfxState* gfx, Texture2D const* texture)
{
    gfx->context->PSSetShaderResources(0, 1, &texture->srv);
    gfx->context->VSSetShaderResources(0, 1, &texture->srv);
    //gfx->context->CSSetShaderResources(0, 1, &texture.srv);
}

///
void vertex_buffer_create(GfxState* gfx, ID3D11Buffer **out_buffer, u32 size_bytes) 
{
    D3D11_BUFFER_DESC bd{};
    bd.ByteWidth = size_bytes;
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.MiscFlags = 0;
    bd.StructureByteStride = 0;
    HRESULT hres = gfx->device->CreateBuffer(&bd, nullptr, out_buffer);
    Q_ASSERT(SUCCEEDED(hres));
}

void vertex_buffer_bind(GfxState* gfx, ID3D11Buffer **buffers,
                                 u32 buff_count,
                                 u32 stride,
                                 u32 offset) 
{
    gfx->context->IASetVertexBuffers(0, 1, buffers, &stride, &offset);
}

GfxMappedMemory vertex_buffer_map_for_write(GfxState* gfx, ID3D11Buffer* buffer)
{
    D3D11_MAPPED_SUBRESOURCE mapped{0};
    GfxMappedMemory result{};

    gfx->context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped); 
    result.mem = mapped.pData;
    result.row_pitch = mapped.RowPitch;
    result.depth_pitch = mapped.DepthPitch; 
    return result; 
}

GfxMappedMemory index_buffer_map_for_write(GfxState* gfx, ID3D11Buffer* buffer)
{
    D3D11_MAPPED_SUBRESOURCE mapped{0};
    GfxMappedMemory result{};

    gfx->context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped); 
    result.mem = mapped.pData;
    result.row_pitch = mapped.RowPitch;
    result.depth_pitch = mapped.DepthPitch; 
    return result; 
}

void vertex_buffer_unmap(GfxState* gfx, ID3D11Buffer* buffer)
{
    gfx->context->Unmap(buffer, 0);

}

void index_buffer_unmap(GfxState* gfx, ID3D11Buffer* buffer) 
{
    gfx->context->Unmap(buffer, 0);

}

void index_buffer_create(GfxState* gfx, ID3D11Buffer **out_buffer, u32 num_indices) 
{
    D3D11_BUFFER_DESC bd{};
    bd.ByteWidth = sizeof(u32) * num_indices;
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.MiscFlags = 0;
    bd.StructureByteStride = 0;
    HRESULT hres = gfx->device->CreateBuffer(&bd, nullptr, out_buffer);
    Q_ASSERT(SUCCEEDED(hres));
} 

void index_buffer_bind(GfxState* gfx, ID3D11Buffer *buffer)
{
    gfx->context->IASetIndexBuffer(buffer, DXGI_FORMAT_R32_UINT, 0);
}

RenderTarget render_target_create(GfxState* gfx, RenderTargetSettings settings)
{
    RenderTarget target{};

    D3D11_TEXTURE2D_DESC texture_desc{};
    D3D11_RENDER_TARGET_VIEW_DESC view_desc{};
    D3D11_SHADER_RESOURCE_VIEW_DESC shader_view_desc{};
    
    texture_desc.Width =  settings.width;
    texture_desc.Height = settings.height;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    if(gfx->device->CreateTexture2D(&texture_desc, 0, &target.render_target) >= 0)
    {
        view_desc.Format = texture_desc.Format;
        view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        view_desc.Texture2D.MipSlice = 0;
        if(gfx->device->CreateRenderTargetView(target.render_target, &view_desc, &target.render_target_view) >= 0 )
        { 
            shader_view_desc.Format = texture_desc.Format;
            shader_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            shader_view_desc.Texture2D.MostDetailedMip = 0;
            shader_view_desc.Texture2D.MipLevels = 1;
            if(gfx->device->CreateShaderResourceView(target.render_target, &shader_view_desc, &target.srv) >= 0)
            {
                target.min = glm::vec2{0, 0};
                target.max = glm::vec2{(f32)settings.width, (f32)settings.height};
                target.format = settings.format;
                return target;
            }
            else
            {
                return RenderTarget{}; 
            }
        }
        else
        { 
            return RenderTarget{}; 
        } 
    }
    else
    { 
        return RenderTarget{}; 
    }
}

b8 render_texture_create(struct GfxState* gfx,
                        RenderTargetSettings* settings,
                        RenderTarget* target)
{
    D3D11_TEXTURE2D_DESC texture_desc{};
    D3D11_RENDER_TARGET_VIEW_DESC view_desc{};
    D3D11_SHADER_RESOURCE_VIEW_DESC shader_view_desc{};


    texture_desc.Width =  settings->width;
    texture_desc.Height = settings->height;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    HRESULT r = gfx->device->CreateTexture2D(&texture_desc, 
                                 nullptr, 
                                 &target->render_target);
    LOG_COM(r);

    view_desc.Format = texture_desc.Format;
    view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    view_desc.Texture2D.MipSlice = 0;
    
    r = gfx->device->CreateRenderTargetView(target->render_target,
            &view_desc, 
            &target->render_target_view);

    shader_view_desc.Format = texture_desc.Format;
    shader_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shader_view_desc.Texture2D.MostDetailedMip = 0;
    shader_view_desc.Texture2D.MipLevels = 1;

    r = gfx->device->CreateShaderResourceView(target->render_target, &shader_view_desc, &target->srv);
    LOG_COM(r);
    return SUCCEEDED(r);
}


b8 render_target_resize(RenderTarget* target, GfxState* gfx, f32 new_width, f32 new_height)
{
    D3D11_TEXTURE2D_DESC texture_desc{};
    D3D11_RENDER_TARGET_VIEW_DESC view_desc{};
    D3D11_SHADER_RESOURCE_VIEW_DESC shader_view_desc{};

    if(new_width >= 1.0f && new_height >= 1.0f)
    {
        /* release old resources */
        if(target->render_target)
        {
            target->render_target->Release(); 
            target->render_target = 0;

        }

        if(target->render_target_view)
        {
            target->render_target_view->Release(); 
            target->render_target_view = 0;
        }

        if(target->srv)
        {
            target->srv->Release(); 
            target->srv = 0;
        }

        texture_desc.Width = new_width;
        texture_desc.Height = new_height;
        texture_desc.Format = target->format;
        texture_desc.MipLevels = 1;
        texture_desc.ArraySize = 1;
        texture_desc.SampleDesc.Count = 1;
        texture_desc.Usage = D3D11_USAGE_DEFAULT;
        texture_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        texture_desc.CPUAccessFlags = 0;
        texture_desc.MiscFlags = 0;

        if(gfx->device->CreateTexture2D(&texture_desc, 0, &target->render_target) >= 0)
        {
            view_desc.Format = texture_desc.Format;
            view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            view_desc.Texture2D.MipSlice = 0;
            if(gfx->device->CreateRenderTargetView(target->render_target, &view_desc, &target->render_target_view) >= 0)
            {
                shader_view_desc.Format = texture_desc.Format;
                shader_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                shader_view_desc.Texture2D.MostDetailedMip = 0;
                shader_view_desc.Texture2D.MipLevels = 1;
                if(gfx->device->CreateShaderResourceView(target->render_target, &shader_view_desc, &target->srv) >= 0)
                {
                    return true;
                }
            }
            else
            {
                return false;
            } 
        }
        else
        {
            return false;
        } 
    }
    else
    {
        LOG_WARNING("render target dimensions ({} {}) are too small", new_width, new_height);
        return false;
    } 

}

b8 texture_create(GfxState* gfx, TextureParams const &params, Texture2D &out_texture) 
{
    HRESULT check;
    D3D11_TEXTURE2D_DESC tex_desc{};
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    D3D11_SUBRESOURCE_DATA sr{};
    ID3D11Texture2D *texture{};

    // make sure requested width and height are acceptable
    if (!(params.desired_width < D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION &&
                params.desired_height < D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION)) 
    {
        LOG_WARNING("requested texture size is too big: w= {} h= {}",
                params.desired_width,
                params.desired_height);
        return false;
    }

    out_texture.width = params.desired_width;
    out_texture.height = params.desired_height;

    // create the texture
    tex_desc.Width = params.desired_width;
    tex_desc.Height = params.desired_height;
    tex_desc.MipLevels = 1;
    tex_desc.ArraySize = 1;
    tex_desc.Format = (DXGI_FORMAT)params.format;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.Usage = params.usage;
    tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    tex_desc.CPUAccessFlags = params.cpu_flags;

    sr.pSysMem = params.initial_data;
    sr.SysMemPitch = tex_desc.Width * 4;
    sr.SysMemSlicePitch = 0;

    if (params.initial_data == nullptr) 
    {
        check = gfx->device->CreateTexture2D(&tex_desc, nullptr, &out_texture.texture);
    } 
    else 
    {
        check = gfx->device->CreateTexture2D(&tex_desc, &sr, &out_texture.texture);
    }

    if (!SUCCEEDED(check)) 
    {
        LOG_COM(check);
        return false;
    }

    srv_desc.Format = (DXGI_FORMAT)params.format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = tex_desc.MipLevels;
    srv_desc.Texture2D.MostDetailedMip = 0;
    check = gfx->device->CreateShaderResourceView(out_texture.texture,
            &srv_desc,
            &out_texture.srv);

    if (!SUCCEEDED(check)) {
        LOG_COM(check);
        return false;
    }

    return true;
}

void texture_update_subregion(GfxState* gfx, Texture2D& texture, 
        u32 subresource, 
        D3D11_BOX* region, 
        void* src_data, 
        u32 src_pitch, 
        u32 src_depth_pitch) 
{
    gfx->context->UpdateSubresource(texture.texture, subresource, region, src_data, src_pitch, src_depth_pitch);
}

void texture_bind(GfxState* gfx, Texture2D* texture)
{
    ID3D11ShaderResourceView* psrv[1] = {nullptr}; // clear the 1st slot
    if(texture == nullptr) 
    {
        gfx->context->PSSetShaderResources(0, 1, (ID3D11ShaderResourceView**)&psrv); 
    } 
    else 
    { 
        gfx->context->PSSetShaderResources(0, 1, &texture->srv);
    }
}

void gfx_draw_fullscreen_quad(GfxState* gfx) 
{
    //gfx->context->VSSetConstantBuffers(0, 1, gfx->scene_constant_buffer.GetAddressOf());
    //gfx->context->PSSetConstantBuffers(0, 1, gfx->scene_constant_buffer.GetAddressOf());
    gfx->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    shader_bind(gfx, &gfx->shader_fullscreen_quad);
   // pixel_shader_bind(gfx->ps_fullscreen_quad.Get());
   // vertex_shader_bind(this->vs_fullscreen_quad.Get());
    gfx->context->Draw(4, 0); 
}

