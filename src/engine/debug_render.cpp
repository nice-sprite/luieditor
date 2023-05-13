#include "debug_render.h"
#include <d3d11.h>
#include <d3dcommon.h>
#include "renderer.h"

DebugRenderSystem* g_debug_system;

void debug_system_init(GfxState* gfx) 
{
    g_debug_system = new DebugRenderSystem;
    const char* debug_line_shader_path = "w:/priscilla/src/hlsl" "debug_line.hlsl";
   // GfxShaderProgram line_shader{};
   // GfxShaderCreateParams settings{};
   // settings.shader_id = "line_shader";
   // settings.source_path = Files::get_shader_root() / "debug_line.hlsl";
   // settings.input_element_desc = nullptr;
   // settings.input_element_count = 0;
   // settings.hotreload_enable = false;
   // settings.precompiled = false;

   // if(shader_program_create(gfx, settings, &g_debug_system->line_shader))
   // {
   //     
   // }

    //vertex_buffer_create(gfx, &this->vertex_buffer, MaxDebugLines * 2 * sizeof(VertexPosColor));
    //renderer.create_vertex_shader(debug_line_shader_path,
    //        VertexPosColor::layout(),
    //        &line_vertex_shader,
    //        &vertex_layout);
    //renderer.create_pixel_shader(debug_line_shader_path, &line_pixel_shader);
    // TODO PICK UP 
}

void debug_system_begin_frame()
{   
     
}

void debug_system_end_frame() 
{ 
    g_debug_system->n_lines = 0; 
}

void debug_line(glm::vec3 to, glm::vec3 from, glm::vec4 color)
{
    if (g_debug_system->n_lines < MaxDebugLines) 
    {
        g_debug_system->lines[g_debug_system->n_lines].begin = to;
        g_debug_system->lines[g_debug_system->n_lines].end = from;
        g_debug_system->lines[g_debug_system->n_lines].color = color;
        ++g_debug_system->n_lines;
    }
}

void debug_ray(Ray* ray, glm::vec4 color, f32 length)
{
    debug_line(ray->origin, ray->origin + (ray->direction * length), color);
}

void DebugRenderSystem::update(GfxState* renderer) 
{
    // renderer.update_buffer(
    //         vertex_buffer.Get(),
    //         [=](D3D11_MAPPED_SUBRESOURCE &msr) {
    //         VertexPosColor *vert_mem = (VertexPosColor *)msr.pData;
    //         for (u32 i = 0u; i < n_lines; ++i) {
    //         vert_mem[i * 2 + 0] = VertexPosColor{lines[i].begin, lines[i].color};
    //         vert_mem[i * 2 + 1] = VertexPosColor{lines[i].end, lines[i].color};
    //         }
    //         });
}

void DebugRenderSystem::draw(GfxState* renderer) 
{
    //renderer.set_topology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    //renderer.set_vertex_buffer(vertex_buffer.GetAddressOf(),
    //                           1,
    //                           sizeof(VertexPosColor),
    //                           0);
    //renderer.set_vertex_shader(line_vertex_shader.Get());
    //renderer.set_pixel_shader(line_pixel_shader.Get());
    //renderer.set_input_layout(vertex_layout.Get());
    //renderer.draw(2 * n_lines);
}
