#include "camera_system.h"
#include "engine_math.h"
#include "input_system.h"
#include <glm/ext.hpp>
#include <DirectXMath.h>

CameraSystem camsys_create(
        u32 count_cameras, 
        glm::vec2 viewport_dimensions,
        CameraMovementData default_movement,
        glm::vec3 origin)
{
    CameraSystem cs;
    cs.camera_count = count_cameras;
    cs.active_cam = 0;
    cs.movement_data =  default_movement;
    cs.cameras = new Camera[cs.camera_count];
    cs.viewport_dimensions = viewport_dimensions;
    for(int i = 0; i < cs.camera_count; ++i)
    {
        cs.cameras[i] = camera_create(origin,
                65.0,
                viewport_dimensions,
                1.0f,
                3000.0f,
                CameraMode::CAM_FLYCAM);
    }
    return cs;
} 

void camsys_destroy(CameraSystem* cams)
{
    if(cams->cameras)
    {
        delete cams->cameras;
    }
}

Camera* camsys_get_camera(CameraSystem* cams, i32 index)
{ 
    return &cams->cameras[index]; 
}

Camera* camsys_get_active(CameraSystem* cams)
{ 
    return &cams->cameras[cams->active_cam];
}

glm::mat4 camsys_active_view_projection(CameraSystem* cams)
{
    Camera* cam = camsys_get_active(cams);
    return cam->view_matrix * cam->projection_matrix; 
}

void camsys_on_screen_resize(CameraSystem* cams, glm::vec2 viewport_dimensions)
{
    Camera* cam = camsys_get_active(cams);
    if(cam->mode == CAM_FLYCAM)
    {
        cam->projection_matrix = glm::perspectiveFovLH(
                glm::radians(cam->fov_degrees),
                viewport_dimensions.x, viewport_dimensions.y,
                cam->z_near,
                cam->z_far);
    }
}

// TODO: time_delta is being passed as 0 currently
void camera_flycam(CameraSystem* cam_sys, InputSystem* input, f64 time_delta) 
{
    if(input->viewport_focused)
    {
        const glm::vec4 basis_forward = {0, 0, 1, 0};
        const glm::vec4 basis_up      = {0, 1, 0, 0};
        const glm::vec4 basis_right   = {1, 0, 0, 0};

        Camera* camera = camsys_get_active(cam_sys);
        glm::vec4 camera_target{0, 0, 0, 0};
        glm::vec4 camera_offset{0, 0, 0, 0};
        glm::mat4 rotation = glm::mat4(1.0);

        if (input_held(input, Btn_LeftMouse))
        {
            glm::vec3 mouse = input_rawinput_reading(input);
            camera->pitch += 0.003 * mouse.y;
            camera->yaw += 0.003 * mouse.x;

            // avoid gimbal lock
            camera->pitch = clampf32((-glm::pi<float>()*0.5 + 0.01), (glm::pi<float>()*0.5 - 0.01), camera->pitch);

            if (input_down(input, Btn_W)) 
            {
                camera_offset += glm::vec4(0.0, 0.0, 1.0, 0.0);
            }

            if (input_down(input, Btn_S)) 
            {
                camera_offset += glm::vec4(0.0, 0.0, -1.0, 0.0);
            }

            if (input_down(input, Btn_A)) 
            {
                camera_offset += glm::vec4(-1.0, 0.0, 0.0, 0.0);
            }

            if (input_down(input, Btn_D)) 
            {
                camera_offset += glm::vec4(1.0, 0.0, 0.0, 0.0);
            }
        }

        float movement_speed;
        if (input_down(input, Btn_Shift)) 
        {
            movement_speed = cam_sys->movement_data.fast_speed;
        }
        else if (input_down(input, Btn_Control)) 
        {
            movement_speed = cam_sys->movement_data.slow_speed;
        }
        else
        {
            movement_speed = cam_sys->movement_data.normal_speed; 
        }


        if (input_down(input, Btn_E))
        { 
            camera->origin += (f32)time_delta * movement_speed * glm::vec4(0.0, 1.0, 0.0, 0.0);
        }
        if (input_down(input, Btn_Q))
        { 
            camera->origin += (f32)time_delta * movement_speed * glm::vec4(0.0, -1.0, 0.0, 0.0);
        }

        rotation = glm::rotate(rotation, camera->yaw, basis_up.xyz());

        // recalculaate the right vector 
        // before doing the pitch and roll because otherwise 
        // the camera will rotate around a tilted axis which we don't want
        camera->right =  rotation * basis_right; 

        rotation = glm::rotate(rotation, camera->pitch, basis_right.xyz());
        rotation = glm::rotate(rotation, camera->roll, basis_forward.xyz());

        camera->forward = rotation * basis_forward; 
        // adjusts the movement to be relative to the cameras current rotation
        // giving the effect of movement being relative to where we are actually looking
        glm::vec4 movement = ((f32)time_delta) * (movement_speed * (rotation * camera_offset));

        camera->origin += movement;
        camera_target = camera->forward;
        camera->view_matrix = glm::lookAt(camera->origin.xyz(),
                (camera->origin + camera_target).xyz(),
                camera->up.xyz());

    }
} 

void camera_dolly(CameraSystem* cam_sys, InputSystem* input, f64 time_delta) 
{ 
    if(input->viewport_focused)
    {
        Camera* camera = camsys_get_active(cam_sys);
        glm::vec4 default_forward{0.f, 0.f, 1.f, 0.f};
        glm::vec4 default_right{1.f, 0.f, 0.f, 0.f};
        glm::mat4 rotation = rotate_roll_pitch_yaw(camera);
        glm::vec4 camera_offset{0, 0, 0, 0};
        glm::vec4 camera_target;
        f32 pan_speed = select_movement_speed(cam_sys, input);
        f32 zoom_speed = select_pan_speed(cam_sys, input);

        if (input_down(input, Btn_Control)) 
        { 
            f32 mouse_dx = input_rawinput_reading(input).x;
            f32 mouse_dy = input_rawinput_reading(input).y;
            camera_offset += glm::vec4( -mouse_dx, mouse_dy, 0.0, 0.0);
            camera_offset = pan_speed * camera_offset;
            camera_offset.z = zoom_speed * (input_mouse_wheel(input)/128.f);
        }

        camera->origin += (rotation * camera_offset);
        camera_target = (rotation * default_forward);
        camera->view_matrix = glm::lookAt(camera->origin.xyz(),
                (camera->origin + camera_target).xyz(),
                camera->up.xyz());
    }
}

glm::mat4 rotate_roll_pitch_yaw(Camera* cam)
{
    const glm::vec3 basis_forward = {0, 0, 1};
    const glm::vec3 basis_up      = {0, 1, 0};
    const glm::vec3 basis_right   = {1, 0, 0};
    glm::mat4 rot = glm::mat4(1.0);
    rot = glm::rotate(rot, cam->roll, basis_forward); 
    rot = glm::rotate(rot, cam->pitch, basis_right); 
    rot = glm::rotate(rot, cam->yaw, basis_up); 
    return rot;
}

f32 select_movement_speed(CameraSystem* cam_sys, InputSystem* input)
{
    if (input_down(input, Btn_Shift)) 
    {
        return cam_sys->movement_data.fast_speed;
    }
    else if (input_down(input, Btn_Control)) 
    {
        return cam_sys->movement_data.slow_speed;
    }
    else
    {
        return cam_sys->movement_data.normal_speed; 
    }
}

const char* camera_mode_to_string(CameraMode mode)
{
    switch(mode)
    {
        case CAM_FLYCAM: return "FLYCAM";
        case CAM_DOLLY: return "DOLLYCAM";
        case CAM_DOLLY_ORTHO: return "ORTH_DOLLYCAM";
        default: return "<unk>";
    }
}

Camera camera_create(glm::vec3 origin, f32 fov_in_degrees, glm::vec2 view_dimensions, float z_near, float z_far, CameraMode mode)
{
    Camera cam{};
    cam.origin =    glm::vec4(origin, 0);
    cam.forward =   glm::vec4(0, 0, 1, 0); // forward is +z axis
    cam.up =        glm::vec4(0, 1, 0, 0); // up is +y
    cam.right =     glm::vec4(1, 0, 0, 0); // right is +x
    cam.view_matrix = glm::lookAt(cam.origin.xyz(), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    cam.pitch = 0.0;
    cam.roll = 0.0;
    cam.yaw = 0.0;
    cam.z_near = z_near;
    cam.z_far = z_far;
    cam.fov_degrees = fov_in_degrees;
    cam.view_dimensions = view_dimensions;

    if(mode == CAM_DOLLY_ORTHO)
    {
        // cam.projection_matrix = glm::ortho(view_dimensions.x, view_dimensions.y, z_near, z_far);
    }
    else
    { 
        //        cam.projection_matrix = glm::perspective(
        //                fov_in_degrees,
        //                view_dimensions.x/view_dimensions.y,
        //                z_near, z_far);

        cam.projection_matrix = glm::perspectiveFovLH(
                glm::radians(fov_in_degrees), 
                view_dimensions.x,
                view_dimensions.y,
                z_near, z_far);
    }

    return cam;
}

f32 select_pan_speed(CameraSystem* cam_sys, struct InputSystem* input)
{
    if(input_down(input, Btn_Shift))
    {
        return cam_sys->movement_data.scroll_step_fast;
    }
    else
    {
        return cam_sys->movement_data.scroll_step_normal;
    }

}
