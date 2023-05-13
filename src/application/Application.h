#include "../engine/camera_system.h"
#include "../engine/debug_render.h"
#include "../engine/font_renderer.h"
#include "../engine/input_system.h"
#include "../engine/renderer.h"
#include "../engine/scene.h"
#include "../engine/timer.h"
#include "../engine/win32_lib.h"
#include "../engine/texture.h"

static constexpr auto DefaultAppWidth = 1920;
static constexpr auto DefaultAppHeight = 1080;
static constexpr auto AppTitle = "equinox - by nice_sprite";


LRESULT CALLBACK win32_message_callback(HWND hwnd, 
                                        u32 msg,
                                        WPARAM wparam,
                                        LPARAM lparam);


using PreframeUpdateFn = void (*)(struct AppState* app, f64 frame_time);

// this can probably be a namespace
// also, can I somehow ensure that all the components are 
// contructed and initialized properly? 
// Pretty clear area for improvement!
struct AppState 
{
    std::string name;
    glm::vec2 viewport_dimensions;
    win32::Window window;
    Timer timer;
    f64 last_frame_time;
    u64 frame_counter;
    glm::vec4 clear_color;
    GfxState* gfx;
    CameraSystem camera_system;
    DebugRenderSystem* debug_gfx;
    FontRenderer* font_system;
    ui::SceneDef scene;
    GfxSceneResources scene_render_resources; // TODO maybe call this "render group resources"
    InputSystem input_system;
    RayCaster* ray;
    struct UILayoutSystem* layout_system;
    struct RemoteLUIUpdateSystem* live_game;
    PreframeUpdateFn preframe_fns[32]{}; // can put debug/metric capture shit here, conditional input fns
    Texture2D test_texture;
    assets::AssetDB asset_db;
};

void app_create(AppState* app, HINSTANCE hinst, std::string appname);

/* Handles creating the renderer and renderer-dependent systems
 * This is because a window is required to be created for directX to work  
 * */ 
void app_handle_window_create(AppState* app, 
                              HWND hwnd, 
                              i32 x, 
                              i32 y, 
                              i32 width, 
                              i32 height);

void app_message_loop(AppState* app);
void app_systems_run(AppState* app, f64 delta_time);
void app_update_hooks_run(AppState* app); // allows pre-frame-update hooks to run 
void app_update_hooks_install(PreframeUpdateFn fn, i32 priority);
void app_resize(AppState* app, i32 w, i32 h);
void app_shutdown(AppState* app);
void app_begin_frame(AppState* app);
void app_end_frame(AppState* app);
void app_cbuffer_update(AppState* app);

void input_update(InputSystem* input);

/* processes user interaction w/ the scene 
 * calculates selected, hovered, dragging, etc 
 * */
void scene_run_interaction(ui::SceneDef* scene, InputSystem* input, RayCaster* ray, f32 time_delta);

/* renders the scene */
void scene_render(ui::SceneDef* active_scene, 
        GfxSceneResources* resources,
        GfxState* gfx,
        f32 frame_delta);

/*
 * Render the editor UI for the current scene
 * */
void editor_scene_display(ui::SceneDef* scene);
void editor_main_ui(AppState* app);
void editor_camera_controls(CameraSystem* camera_system);

struct EditorViewpaneData
{
    glm::vec2 vp_min;
    glm::vec2 vp_max; 
    bool focused;
};

EditorViewpaneData editor_viewpane(const char* title, void* image_srv_for_imgui);

void camera_input(CameraSystem* camera_sys, InputSystem* input, f64 time_delta);

