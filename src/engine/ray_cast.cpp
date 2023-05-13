#include "ray_cast.h"
#include "camera_system.h"
// #include <algorithm>
#include "logging.h"
#include <imgui.h>


// can we make the raycaster a per-viewport thing
// since the data is all related
// camera needs to have the correct aspectratio and viewport size,
// ray caster needs the camera for the viewport to shoot rays at it,
// and each viewport should only have 1 scene being rendered to it
RayCaster &RayCaster::instance() {
    static RayCaster rc;
    return rc;
}

//void RayCaster::init(CameraSystem *cam_sys) { this->cam_sys = cam_sys; }

// transforms a world-space vector into screenspace
glm::vec4 RayCaster::project(glm::vec4 world) {

    // glm::vec4 ret = DirectX::XMVector3Project(world,
    //                                      active_viewport.x,
    //                                      active_viewport.y,
    //                                      active_viewport.w,
    //                                      active_viewport.h,
    //                                      0.0,
    //                                      1.0,
    //                                      cam_sys->get_active().get_projection(),
    //                                      cam_sys->get_active().get_view(),
    //                                      XMMatrixIdentity());
    return {};
}

// transforms a screen-space vector (2d xy) into a normalized Ray
Ray RayCaster::picking_ray(glm::vec4 screen) {
    Ray ray;
    // screen = XMVectorSubtract(screen,
    //             glm::vec4F32{active_viewport.x, active_viewport.y, 0, 0});
    // LOG_INFO("mouse pos: {}", screen);
    // glm::vec4 screen_near = XMVectorSetZ(screen, 0.0);
    // glm::vec4 screen_far = XMVectorSetZ(screen, 1.0);
    // ray.origin = unproject(screen_near);
    // glm::vec4 dest = unproject(screen_far);
    // ray.direction = XMVector3Normalize(XMVectorSubtract(dest, ray.origin));
    return ray;
}

glm::vec4 RayCaster::unproject(glm::vec4 xyz) {
    //  return XMVector3Unproject(xyz,
    //                            active_viewport.x,
    //                            active_viewport.y,
    //                            active_viewport.w,
    //                            active_viewport.h,
    //                            0.0,
    //                            1.0,
    //                            cam_sys->get_active().get_projection(),
    //                            cam_sys->get_active().get_view(),
    //                            XMMatrixIdentity());
    return {};
}

bool RayCaster::ray_quad(Ray ray, glm::vec4 quad_bounds) {

    return false;
}

glm::vec4 RayCaster::quad_plane(glm::vec4 quad) {
    return {};
}

void RayCaster::set_viewport(ViewportRegion vp) { active_viewport = vp; }



