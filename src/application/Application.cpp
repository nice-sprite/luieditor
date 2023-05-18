#include "Application.h"
#include "../engine/files.h"
#include "../engine/logging.h"
#include <Tracy.hpp>
#include <Windows.h>
#include <imgui.h>
#include <winuser.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include "../engine/imgui_custom.h"
#include <DirectXMath.h>
#include "../engine/texture.h"

void app_create(AppState* app, HINSTANCE hinst, std::string appname) 
{
    app->window = win32::create_window(hinst,
            appname.c_str(),
            "luieditor",
            DefaultAppWidth,
            DefaultAppHeight,
            win32_message_callback,
            0,
            (void*)app);

}

void app_message_loop(AppState* app) 
{
    MSG msg;
    bool should_close = false;
    while (!should_close) 
    {
        app->timer.begin();
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) 
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) 
            {
                should_close = true;
                break;
            }
        } 
        app_systems_run(app, app->last_frame_time);
        app->timer.end();
        app->last_frame_time = app->timer.elapsed_ms();
    }
}

void app_begin_frame(AppState* app) 
{
    backbuffer_clear(app->gfx, &app->gfx->current_backbuffer, app->clear_color);
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    gfx_begin_frame(app->gfx);
}

void app_end_frame(AppState* app) 
{
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    gfx_end_frame(app->gfx);
    gfx_present(app->gfx, true);
}

void app_cbuffer_update(AppState* app) 
{
    /* TODO rename */
    //app->gfx->constant_buffer_map();
    /* TODO push constant buffers */
    //app->gfx->constant_buffer_unmap();
}

void app_systems_run(AppState* app, f64 delta_time) 
{
    input_begin_frame(&app->input_system);
    app_begin_frame(app);

    camera_input(&app->camera_system,
            &app->input_system,
            delta_time); 

    scene_run_interaction(&app->scene,
            &app->input_system,
            app->ray,
            delta_time); 

    /* after everything has run, its time to update the constant buffers for shaders */
    GfxMappedMemory cbuf_mem = gfx_map_global_constants_for_write(app->gfx);
    //app->gfx->global_shader_constants.modelViewProjection = 
    //    glm::transpose(
    //     camsys_active_view_projection(&app->camera_system) 
    //    );

    Camera* active_cam = camsys_get_active(&app->camera_system);

    app->gfx->global_shader_constants.modelViewProjection = active_cam->projection_matrix 
        * active_cam->view_matrix * glm::mat4(1.0f);

    app->gfx->global_shader_constants.timeTickDeltaFrame = glm::vec4(1, 2, 3, 4);
    app->gfx->global_shader_constants.viewportSize = glm::vec4(5, 6, 7, 8);
    memcpy(cbuf_mem.mem, &app->gfx->global_shader_constants, sizeof(GlobalShaderConstants)); 
    gfx_unmap_global_constants(app->gfx);
    gfx_set_constant_buffer(app->gfx, app->gfx->shader_constants_buffer);

    /* start rendering to this target */
    gfx_push_render_texture(app->gfx, &app->scene_render_resources.render_target);
    scene_render(&app->scene,
            &app->scene_render_resources,
            app->gfx,
            app->asset_db,
            delta_time);
    gfx_pop_render_texture(app->gfx); // unbind 


    /* run UI stuff */
    // begin UI pass, render the 
    backbuffer_bind(app->gfx, &app->gfx->current_backbuffer); // 
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    // tell imgui to draw what we rendered in the texture
    // the render target needs feedback for how large the imgui window currently is
    // so make this return the current region and then resize the target 
    EditorViewpaneData viewpane_data = editor_viewpane("Main scene", (void*)app->scene_render_resources.render_target.srv);
    glm::vec2 vp_dims;
    glm::vec2 rt_dims;
    FLOAT2_SUBTRACT(vp_dims, viewpane_data.vp_max, viewpane_data.vp_min);
    FLOAT2_SUBTRACT(rt_dims,
            app->scene_render_resources.render_target.max,
            app->scene_render_resources.render_target.min);

    if(!FLOAT2_EQUAL(rt_dims, vp_dims))
    { 
        if(vp_dims.x >= 1.0f && vp_dims.y >= 1.0f)
        { 
            gfx_deferred_target_resize(app->gfx, &app->scene_render_resources.render_target, vp_dims);
            camsys_on_screen_resize(&app->camera_system, vp_dims); 
        }
    }

    app->scene_render_resources.render_target.min = viewpane_data.vp_min;
    app->scene_render_resources.render_target.max = viewpane_data.vp_max;
    app->input_system.viewport_focused = viewpane_data.focused;

    if(app->input_system.viewport_focused && app->input_system.mouse.left.state != 0)
    {
        input_lock_cursor(&app->input_system);
        input_hide_cursor(&app->input_system);
        //LOG_INFO("HIDING");
    }
    else
    {
        input_unlock_cursor(&app->input_system);
        input_show_cursor(&app->input_system);
        //LOG_INFO("SHOWING");
    }

    editor_main_ui(app);
    input_end_frame(&app->input_system);
    app_end_frame(app);
}

void app_shutdown(AppState* app) 
{
    scene_gfx_destroy(&app->scene_render_resources);
    scene_destroy(&app->scene);
    camsys_destroy(&app->camera_system);
    input_destroy(&app->input_system);
    delete app->gfx;
    delete app->debug_gfx;
    delete app->font_system;
    delete app->ray;
    // delete app->layout_system ;
    // delete app->live_game ;
}

void app_handle_window_create(AppState* app, 
        HWND hwnd, 
        i32 x, 
        i32 y, 
        i32 width, i32 height) 
{ 
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)app);

    //app->gfx = new Renderer(hwnd, width, height);

    app->gfx = gfx_start((void*)hwnd, width, height);
    app->input_system = input_create(hwnd, true); 
    app->scene = ui::scene_create(10000);
    app->scene_render_resources = scene_create_render_resource_for_scene(app->gfx, &app->scene);
    app->asset_db = assets::db_init(app->gfx->device);

    app->camera_system = camsys_create(
            16,
            glm::vec2{(f32)width, (f32)height}, 
            CameraMovementData{
                .normal_speed = 5.0, 
                .fast_speed = 10.0, 
                .slow_speed = 1.0, 
                .smoothness = 0.0, 
                .inertia = 5.0,
                .scroll_step_normal = 25.0,
                .scroll_step_fast = 50.0,
            },
            glm::vec3(scene_calculate_root_center(&app->scene), 800)
    );

    app->debug_gfx = new DebugRenderSystem();
    //app->font_system = new FontRenderer(app->gfx);
    app->font_system = nullptr;
    app->ray = new RayCaster();
    app->layout_system = nullptr;
    app->live_game = nullptr;
    app->clear_color = {0.5, 0.5, 0.5, 1.0};

    // create test texture
    bool loaded = 0;
    fsapi::Path path = fsapi::exe_dir().parent_path() / "resource/spr_bw_226.png";
    app->test_texture = assets::create_texture(std::move(assets::load_image(path)), app->gfx->device, &loaded);
    if(loaded)
    {
        LOG_INFO("Texture should be good!");
    }
    else
    {
        LOG_WARNING("TEXTURE NOT GOOD!");
    }

    /* setup imgui */
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;
    LOG_INFO("imgui version: {} docking?: {}",
            ImGui::GetVersion(),
            (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable) != 0);

    ImGui_ImplDX11_Init(app->gfx->device, app->gfx->context);
    ImGui_ImplWin32_Init(hwnd);
}

void app_resize(AppState* app, i32 w, i32 h) 
{
    LOG_INFO("Resize to: {} {}", w, h); 
    backbuffer_resize(app->gfx, &app->gfx->current_backbuffer, w, h, false);
}

void editor_main_ui(AppState* app) 
{
    assets::AssetDB* db = app->asset_db;
    static b8 a = true;
    ImGui::ShowDemoWindow(&a);
    //editor_scene_display(&app->scene);
    ui::editor_update(&app->scene, app->test_texture);
    editor_camera_controls(&app->camera_system);
    input_debug_ui(&app->input_system);

    //assets::editor_image_picker(db);
    assets::BrowserInteraction browser_interact = assets::editor_draw_browser(db);

    // 0 means nothing was selected
    if(browser_interact.selected_asset != 0)
    {
        if(app->scene.tree_state.selected_element)
        {
            ui::UIElement* element = app->scene.tree_state.selected_element;
            LOG_INFO("assigned element {} with asset {}", 
                    element->name,
                    db->assets[browser_interact.selected_asset].name.c_str());
            element->asset_id = browser_interact.selected_asset;
        }
    }


    if(ImGui::Begin("Fonts")) 
    {
        // TODO 
        // open file dialoge 
        // offload to a thread so rendering isnt blocked 
        if(ImGui::Button("Add font")) 
        { 
            ImGui::Text("Not implemented yet"); 
        } 
    }
    ImGui::End();
}

EditorViewpaneData editor_viewpane(const char* title, void* image_srv_for_imgui)
{
    EditorViewpaneData result{};
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0, 0.0));

    ImVec2 min;
    ImVec2 max;
    ImVec2 viewpane_size;

    if(ImGui::Begin(title))
    {
        //result.focused = ImGui::IsWindowFocused() && !ImGui::IsItemFocused() && !ImGui::IsItemActive(); // the is item focused here refers to the windows titlebar


        min = ImGui::GetWindowContentRegionMin();
        max = ImGui::GetWindowContentRegionMax();
        viewpane_size = ImGui::GetContentRegionAvail();
        ImVec2 wpos = ImGui::GetWindowPos();

        min.x += wpos.x;
        min.y += wpos.y;

        max.x += wpos.x;
        max.y += wpos.y;

        COPY_XY(result.vp_min, min);
        COPY_XY(result.vp_max, max);

        // ImGui::GetForegroundDrawList()->AddRect(min, max, IM_COL32(0xff, 0, 0, 0xff)); 
        ImRect aabb(min, max);
        bool hovered, held;
        ImGui::ButtonBehavior(aabb, ImGui::GetID("##viewpane"), &hovered, &held);
        ImGui::Image(image_srv_for_imgui, viewpane_size);

        result.focused = hovered || held;
    }
    else  // if the window was not even being drawn it cant be active!
    {
        result.focused = false;
    }
    ImGui::End();

    ImGui::PopStyleVar();
    return ( result );
}

void editor_scene_draw_tree_nodes(ui::UIElement* node)
{
    if(node == 0) return;

    ui::UIElement* cur_node = node;
    while(cur_node != 0)
    {
    }
}


void editor_camera_controls(CameraSystem* camera_system) {
    Camera* cam = camsys_get_active(camera_system);
    if(ImGui::Begin("Camera settings")) {
        ImGui::InputFloat("Movement speed", &camera_system->movement_data.normal_speed);
        ImGui::InputFloat("Fast speed",     &camera_system->movement_data.fast_speed);
        ImGui::InputFloat("Slow speed",     &camera_system->movement_data.slow_speed);
        ImGui::InputFloat("Smoothness",     &camera_system->movement_data.smoothness);
        ImGui::InputFloat("Inertia",        &camera_system->movement_data.inertia);
        ImGui::Separator();
        ImGui::Text("pitch: %f yaw: %f roll: %f", cam->pitch, cam->yaw, cam->roll);
        for(int i = 0; i < CameraMode::COUNT; ++i)
        {
            b8 pressed = ImGui::Button(camera_mode_to_string(CameraMode(i)));
            ImGui::SameLine();
            if(pressed)
            {
                cam->mode = CameraMode(i);
            }
        }

        ImGui::Separator();
        glm::mat4 view_tpose = glm::transpose(cam->view_matrix);
        glm::mat4 proj_tpose = glm::transpose(cam->projection_matrix);

        ImGui::Text("view matrix");
        ImGui::Mat4x4(cam->view_matrix);
        ImGui::Separator();
        ImGui::Mat4x4(view_tpose);

        ImGui::Separator();
        ImGui::Text("perspective matrix");
        ImGui::Mat4x4(cam->projection_matrix);
        ImGui::Separator();
        ImGui::Mat4x4(proj_tpose);
    }
    ImGui::End();
}

// TODO can probably extract the correct input vectors here
// then pass to the function
// especially nice once keybinds are added, that way the 
// camera_x functions only have to do math and nothing else
void camera_input(CameraSystem* camera_sys, InputSystem* input, f64 time_delta) 
{
    //if(input->imgui_active)
    //   return;

    Camera* active_camera = camsys_get_active(camera_sys);

    if(active_camera->mode == CAM_FLYCAM) 
    {
        camera_flycam(camera_sys, input, time_delta);
    } 
    else if(active_camera->mode == CAM_DOLLY) 
    {
        camera_dolly(camera_sys, input, time_delta);
    }
}

// might want to pass in a camera here so we can add features like "align camera to this element"
void scene_run_interaction(ui::SceneDef* scene,
        InputSystem* input,
        RayCaster* ray,
        f32 time_delta) 
{

} 

VertexPosColorTexcoord uielement_to_vertex(f32 x, f32 y, f32 z, glm::vec4 color, f32 u, f32 v)
{
    VertexPosColorTexcoord result;
    result.color = color;
    result.pos = glm::vec3{x, y, 0.0f};
    result.texcoord = glm::vec2{u, v};
    return result;
}

void scene_render(ui::SceneDef* active_scene, 
        GfxSceneResources* resources, 
        GfxState* gfx, 
        assets::AssetDB* assets,
        f32 frame_delta) 
{ 
    static bool success = 0;

    static Texture2D test = assets::create_texture(std::move(assets::load_image(fsapi::exe_dir().parent_path() / "resource/spr_bw_226.png")), gfx->device, &success);
    Q_ASSERT(success);


    GfxMappedMemory vertex_out = vertex_buffer_map_for_write(gfx, resources->vertex_buffer);
    GfxMappedMemory index_out =  index_buffer_map_for_write(gfx, resources->index_buffer);
    VertexPosColorTexcoord* vout = (VertexPosColorTexcoord*)vertex_out.mem;
    u32* iout = (u32*)index_out.mem;
    for(i32 i = 0; i < active_scene->used_element_count; ++i)
    {
        ui::UIElement* elem = &active_scene->elements[i];
        ui::UIElementBoundingBox box = elem->box;

        if(elem->type == ui::UIElementType::Image)
        {
            // if needed maybe we can create the texture?
            Texture2D* image = assets::resolve_image(assets, elem->asset_id);
            if(image)
            {
                gfx_texture_bind(gfx, image);

            }
            // bind the texture
            //gfx_texture_bind(gfx, &test);
        }

        vout[i * 4 + 0] = uielement_to_vertex(box.left, box.top,    (f32)elem->priority, elem->color, 0.0, 0.0);
        vout[i * 4 + 1] = uielement_to_vertex(box.right,box.top,    (f32)elem->priority, elem->color, 1.0, 0.0);
        vout[i * 4 + 2] = uielement_to_vertex(box.left, box.bottom, (f32)elem->priority, elem->color, 0.0, 1.0);
        vout[i * 4 + 3] = uielement_to_vertex(box.right,box.bottom, (f32)elem->priority, elem->color, 1.0, 1.0);

        iout[i * 6 + 0] = i * 4 + 2;
        iout[i * 6 + 1] = i * 4 + 3;
        iout[i * 6 + 2] = i * 4 + 1;
        iout[i * 6 + 3] = i * 4 + 2;
        iout[i * 6 + 4] = i * 4 + 1;
        iout[i * 6 + 5] = i * 4 + 0;
    }

    vertex_buffer_unmap(gfx, resources->vertex_buffer);
    index_buffer_unmap(gfx, resources->index_buffer);

    gfx_set_topology(gfx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    vertex_buffer_bind(gfx, &resources->vertex_buffer, 1, sizeof(VertexPosColorTexcoord), 0);
    index_buffer_bind(gfx, resources->index_buffer);
    shader_bind(gfx, &resources->scene_shader);
    gfx_draw_indexed(gfx, active_scene->used_element_count * 6);

    //gfx->set_topology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //gfx->input_layout_bind(resources->vertex_layout);
    //gfx->vertex_buffer_bind(&resources->vertex_buffer, 1, sizeof(VertexPosColorTexcoord), 0);
    //gfx->index_buffer_bind(resources->index_buffer);

    //gfx->texture_bind(nullptr);
    //gfx->vertex_shader_bind(resources->vertex_shader);
    //gfx->pixel_shader_bind(resources->pixel_shader);

    //gfx->draw_indexed(active_scene->element_count * 6);

}

LRESULT CALLBACK win32_message_callback(HWND hwnd, 
        u32 msg,
        WPARAM wparam,
        LPARAM lparam) 
{

    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
        return true;

    if(msg == WM_CREATE) 
    {
        CREATESTRUCT* create_info = (CREATESTRUCT*)lparam;
        AppState* app_ptr = (AppState*)create_info->lpCreateParams;
        app_handle_window_create(app_ptr, 
                hwnd, 
                create_info->x, 
                create_info->y, 
                create_info->cx, 
                create_info->cy);

    }

    AppState* app = (AppState*)GetWindowLongPtr(hwnd, GWLP_USERDATA); 

    if (app) 
    {
        // NOTE: this handles mouse and keyboard, which can actually be moved to the PeekMessage loop if needed
        input_process_win32_message(&app->input_system, Win32WndProcData{hwnd, msg, lparam, wparam});
        if (msg == WM_DESTROY) 
        {
            PostQuitMessage(0);
            return 0;
        } 
        else if(msg == WM_SIZE) 
        {
            u16 new_width = LOWORD(lparam);
            u16 new_height = HIWORD(lparam);
            app_resize(app, new_width, new_height);
        }
    }
    else 
    {
        LOG_WARNING("app invalid"); 
    } 
    return DefWindowProc(hwnd, msg, wparam, lparam); 
}

#define DEBUG_IMGUI_WINDOW 1
