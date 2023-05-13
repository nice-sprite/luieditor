//
// Created by coxtr on 12/14/2021.
//

#include "debug_render.h"
#include "font_renderer.h"
#include "input_system.h"
#include "ray_cast.h"
#include "scene.h"
#include "texture.h"
#include <d3dcommon.h>
#include <filesystem>
#include <random>
#include <windows.h>
#include "files.h"
#include "imgui_custom.h"

#include "imgui_internal.h"

using namespace std::string_literals;

GfxSceneResources scene_create_render_resource_for_scene(GfxState* gfx_state, ui::SceneDef* scene) 
{ 
    GfxSceneResources render_resources{}; 

    // TODO make a "Material" def and live shader reload
    //ID3D11VertexShader* vertex_shader;
    //ID3D11PixelShader* pixel_shader;
    const char*  quad_shader = "w:/priscilla/src/hlsl/" "TexturedQuad.hlsl";
    Q_ASSERT(fsapi::file_exists(quad_shader));

    Q_ASSERT(scene->max_element_count > 0);

    /* calculate allocation sizes for the gpu buffers */ 
    // TODO: check to make sure sure if these values *have* to be 16 or 64 byte aligned 
    u32 size_of_vtx_buff_in_bytes = scene->max_element_count * sizeof(VertexPosColorTexcoord) * 4; 
    u32 count_indices_to_allocate = scene->max_element_count * scene->indices_per_element; 

    vertex_buffer_create(gfx_state, &render_resources.vertex_buffer, size_of_vtx_buff_in_bytes);
    index_buffer_create(gfx_state, &render_resources.index_buffer, count_indices_to_allocate);

    fsapi::FileReadResult shader_code = fsapi::read_entire_file(quad_shader);

    render_resources.scene_shader = shader_program_create(gfx_state, GfxShaderCreateParams{
            .source_code_length = (u32)shader_code.file_size_bytes,
            .source_code = (char*)shader_code.buffer,
            .source_code_path_length =0,
            .source_code_path = 0,
            .shader_name = (char*)"Scene_Textured_Quad_Shader",
            .input_element_count = VertexPosColorTexcoord::layout().element_count,
            .input_element_desc = VertexPosColorTexcoord::layout().element_desc,
            .hotreload_enable = false,
            .precompiled = false, 
    });

    RenderTargetSettings target_settings{};
    target_settings.format = DXGI_FORMAT_R8G8B8A8_UNORM;

    /*  width and height should be the dimensions of the imgui window's drawable region
        passing that information *here* is pretty difficult, so just create *any* arbitrary
        size that is valid, and then we can just wait one frame for the size to get updated
        by the main render/resize code, after the imgui window exists.
     */
    target_settings.width  = 800;
    target_settings.height = 600; 

    render_resources.render_target = render_target_create(gfx_state, target_settings);
    Q_ASSERT(render_resources.render_target.srv != 0);
    Q_ASSERT(render_resources.render_target.render_target != 0);
    Q_ASSERT(render_resources.render_target.render_target_view != 0); 

    fsapi::free_file(shader_code);
    return render_resources;
}

namespace ui 
{
    UIElement* alloc_element(SceneDef* scene)
    {
        UIElement* elem = 0;

        if(scene->used_element_count < scene->max_element_count)
        {
            elem = &scene->elements[scene->used_element_count];
            scene->used_element_count += 1;
            return elem;
        }
        else
        {
            LOG_FATAL("SceneDef::elements is full! Allocation failed");
            return 0;
        }
    }

    UIElement* add_sibling(SceneDef* scene, UIElement* other_sibling)
    {
        UIElement *elem = make_element(scene, "elem", 50, 100, 50, 100, 0, glm::vec4(0.5, 0.5, 0.5, 1.0));
        UIElement* end = other_sibling;
        while(end->next_sibling != 0)
        {
            end = end->next_sibling;
        }
        end->next_sibling = elem;
        return elem;
    }

    UIElement* add_child(SceneDef* scene, UIElement* parent)
    {
        UIElement *child = make_element(scene, "elem", 50, 100, 50, 100, 0, glm::vec4(0.5, 0.5, 0.5, 1.0));
        child->type = UIElementType::Image;

        child->parent = parent;
        if(parent->first_child)
        {
            UIElement* sibling = parent->first_child;
            // insert in sorted order by priority
            while(1)
            {
                bool has_priority = 0;
                if(sibling)
                {
                    has_priority = sibling->priority <= child->priority;
                }
                if(!has_priority)
                {
                    break;
                }
                sibling = sibling->next_sibling;
            }

            if(sibling)
            {
                if(sibling->previous_sibling) // inserting in the middle of the linked list
                {
                    child->previous_sibling = sibling->previous_sibling;
                    child->previous_sibling->next_sibling = child;
                }
                else
                {
                    child->first_child = child;

                }
                child->previous_sibling = sibling;
                sibling->previous_sibling = child;
            }
            else
            {
                child->previous_sibling = parent->last_child;
                parent->last_child->next_sibling = child;
                parent->last_child = child;
            }
        }
        else
        {
            Q_ASSERTMSG(parent->last_child == nullptr, "expected parent->last_child == nullptr but was not!");
            parent->first_child = child;
            parent->last_child = child;
        }
        return child;
    }

    void DEBUG_add_debug_elements(SceneDef* scene)
    {
        for(int i = 0; i < 10; ++i)
        {
            UIElement* child = add_child(scene, scene->root_element);
            child->name += std::to_string(i);
        }
    }


    SceneDef scene_create(u32 max_element_count)
    {
        SceneDef scene{}; 
        scene.max_element_count = max_element_count;
        scene.bytes_per_element = max_element_count * sizeof(UIElement);
        scene.indices_per_element = 6;
        scene.used_element_count = 0;
        scene.elements = (UIElement*)VirtualAlloc(0, scene.max_element_count * scene.bytes_per_element, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
        scene_create_root(&scene, "root0", 0.f, 0.f, 1920.f, 1080.f, glm::vec4{1, 1, 1,1});

        DEBUG_add_debug_elements(&scene);
        //for (int i = 0; i < 25; ++i)
        //{
        //    scene_new_element(&scene, -500, 500, -500, 500, 1, glm::vec4(1, 1, 1, 1));
        //}
        return scene;
    }

    void scene_destroy(SceneDef* scene)
    {
        if(scene->elements)
        {
            VirtualFree(scene->elements, 0, MEM_RELEASE);
        } 
    }

    void scene_create_root(SceneDef* scene, std::string name, f32 x, f32 y, f32 width, f32 height, glm::vec4 color)
    {
        scene->root_element = make_element(scene, name, x, x+width, y, y+height, 0, color);
    }

    // allocate the new element from our element pool 
    // and return a pointer to it
    // can fail if the pool has no free UIElements in it 
    UIElement* make_element(SceneDef* scene, 
            std::string name, 
            f32 left,
            f32 right,
            f32 top,
            f32 bottom,
            i32 layer,
            glm::vec4 color)
    {

        if(UIElement* elem = alloc_element(scene))
        {
            *elem = UIElement {
                .type = UIElementType::Element,
                .parent = 0,
                .previous_sibling = 0,
                .next_sibling = 0,
                .first_child = 0,
                .last_child = 0,
                .name = name,
                .box = UIElementBoundingBox {
                    .left = left, 
                    .right = right, 
                    .top = top, 
                    .bottom = bottom
                },
                .color = color,
                .rotation_x = 0.0f,
                .rotation_y = 0.0f,
                .rotation_z = 0.0f,
                .font = 0, // 0 init for the union
                .priority = 0,
            };

            return elem;
        }
        else
        {
            return 0;
        }
    }

    glm::vec2 scene_calculate_root_center(SceneDef* scene)
    {
        glm::vec2 center;
        float root_width =   scene->root_element->box.right - scene->root_element->box.left;
        float root_height =  scene->root_element->box.bottom - scene->root_element->box.top;
        center.x = scene->root_element->box.left + (root_width / 2.0f);
        center.y = scene->root_element->box.top + (root_height / 2.0f);
        return center;
    }

    void editor_draw_tree_internal(UIElement* root, ElementTreeState* tree_state)
    {
        if(!root) return;
        UIElement* current = root;
        while(current)
        {
            ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_SpanFullWidth 
                | ImGuiTreeNodeFlags_OpenOnDoubleClick 
                | ImGuiTreeNodeFlags_OpenOnArrow; 

            node_flags |= root->first_child ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_Leaf;
            node_flags |= tree_state->selected_element == current ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None;
            bool node_open = ImGui::TreeNodeEx(current->name.c_str(), node_flags);
            bool clicked = ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen();
            if(clicked)
            {
                tree_state->selected_element = current;
            }

            if(node_open)
            {
                editor_draw_tree_internal(current->first_child, tree_state);
                ImGui::TreePop();
            }
            current = current->next_sibling;
        }
    }

    void editor_update(SceneDef* scene, Texture2D& texture)
    {
        if(ImGui::Begin("Element Tree"))
        {
            editor_draw_tree_internal(scene->root_element, &scene->tree_state);
        }
        ImGui::End();

        if(ImGui::Begin("Properties"))
        {
            if(UIElement* elem = scene->tree_state.selected_element)
            {
                ImGui::Text("selected_element: %s", elem->name.c_str());
                ImGui::Columns(2, 0, 0);
                {
                    ImGui::InputFloat_LeftLabel_Handle("Left", &elem->box.left);
                    ImGui::NextColumn();

                    ImGui::InputFloat_LeftLabel_Handle("Right", &elem->box.right);
                    ImGui::NextColumn();

                    ImGui::InputFloat_LeftLabel_Handle("Top", &elem->box.top);
                    ImGui::NextColumn();

                    ImGui::InputFloat_LeftLabel_Handle("bottom", &elem->box.bottom);
                    ImGui::NextColumn();

                    ImGui::Columns(3, 0, 0);

                    ImGui::InputFloat_LeftLabel_Handle("rotX", &elem->rotation_x);
                    ImGui::NextColumn();

                    ImGui::InputFloat_LeftLabel_Handle("rotY", &elem->rotation_y);
                    ImGui::NextColumn();

                    ImGui::InputFloat_LeftLabel_Handle("rotZ", &elem->rotation_z);
                    ImGui::NextColumn();
					ImGui::Separator(); 
                }

                ImGui::Columns(1);
                ImGui::Separator();

                ImGui::Text("Material Settings");
                ImGui::ColorEdit4("color", &elem->color.x);

                if(ImGui::ImagePickerButton("image", (void*)texture.srv, ImVec2(32, 32)))
                {
                    // TODO pop open the image picker!
                    // TODO make this a drag/drop sink so that users 
                    // can drag + drop their images onto the button and it will work!
                    //ImGui::Text("OH MAH GAIIIDDD!!!");
                }

            }
        }
        ImGui::End();
    }
}

void scene_gfx_destroy(GfxSceneResources* gfx_resources) 
{

    safe_release(&gfx_resources->index_buffer);
    safe_release(&gfx_resources->vertex_buffer);
    safe_release(&gfx_resources->vertex_layout);
    shader_destroy(&gfx_resources->scene_shader);
    // safe_release(&gfx_resources->pixel_shader);
    // safe_release(&gfx_resources->vertex_shader);
}
