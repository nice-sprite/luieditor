#pragma once
#include "../defines.h"
#include <glm/glm.hpp>
#include "engine_math.h"

enum CameraMode : i32 
{
    CAM_FLYCAM,
    CAM_DOLLY,
    CAM_DOLLY_ORTHO,
    COUNT
};

const char* camera_mode_to_string(CameraMode mode);

struct Camera 
{
    float pitch;
    float yaw;
    float roll;
    float z_near; 
    float z_far;
    float fov_degrees;
    glm::vec2 view_dimensions;
    glm::vec4 forward, up, right;
    glm::vec4 origin;
    glm::mat4 rotation;
    glm::mat4 view_matrix;
    // can be orthographic or perspective
    glm::mat4 projection_matrix; 
    CameraMode mode;
};

struct CameraMovementData
{
    /* user camera movement settings */
    f32 normal_speed;
    f32 fast_speed;
    f32 slow_speed; 
    f32 smoothness;
    f32 inertia; 
    f32 scroll_step_normal;
    f32 scroll_step_fast;
};

struct CameraSystem 
{ 
    u32 active_cam;
    u32 camera_count;
    Camera* cameras; 
    CameraMovementData movement_data; 
    glm::vec2 viewport_dimensions;
};

/* init `count_cameras` cameras */
CameraSystem camsys_create(u32 count_cameras,
        glm::vec2 viewport_dimensions,
        CameraMovementData default_movement,
        glm::vec3 origin);

void camsys_destroy(CameraSystem* cams);

Camera* camsys_get_camera(CameraSystem* cams, i32 index);
Camera* camsys_get_active(CameraSystem* cams); 
glm::mat4 camsys_active_view_projection(CameraSystem* cams);

void camsys_on_screen_resize(CameraSystem* cams,
        glm::vec2 viewport_dimensions);

Camera camera_create(glm::vec3 origin,
        f32 fov_in_degrees,
        glm::vec2 view_dimensions,
        float z_near,
        float z_far,
        CameraMode mode);


void camera_flycam(CameraSystem* cam_sys, 
        struct InputSystem* input,
        f64 time_delta);

void camera_dolly(CameraSystem* cam_sys, 
        struct InputSystem* input,
        f64 time_delta); 


glm::mat4 rotate_roll_pitch_yaw(Camera* cam);
f32 select_movement_speed(CameraSystem* cam_sys, struct InputSystem* in);
f32 select_pan_speed(CameraSystem* cam_sys, struct InputSystem* input);

