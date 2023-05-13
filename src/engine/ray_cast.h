#pragma once
#include "../defines.h"
#include "engine_math.h"
#include "renderer_types.h"
#include <fmt/format.h>


struct Ray 
{
    glm::vec4 origin;
    glm::vec4 direction;
};

struct RayCaster 
{
    //CameraSystem *cam_sys;
    ViewportRegion active_viewport;

    //void init(CameraSystem *cam_sys);
    RayCaster() = default;
    RayCaster(RayCaster &) = delete;
    static RayCaster &instance();

    void set_viewport(ViewportRegion vp);
    glm::vec4 unproject(glm::vec4 screen);
    Ray picking_ray(glm::vec4 screen);

    glm::vec4 project(glm::vec4 world);

    // helpers

    // get the plane the quad is embedded in
    inline glm::vec4 quad_plane(glm::vec4 quad);

    // intersection tests
    bool ray_quad(Ray ray, glm::vec4 quad_bounds);
    bool ray_volume(Ray min, Ray max, glm::vec4 quad_bounds);
};

//glm::vec2 world_to_screen(glm::vec3 world, f32 width, f32 height, const Camera &camera);

// converts screen coordinates to ray origin and direction
//Ray screen_to_world_ray(float x,
//                        float y,
//                        float width,
//                        float height,
//                        const Camera &camera,
//                        Matrix world);
//
//bool against_quad(Ray const &ray,
//                  float left,
//                  float right,
//                  float top,
//                  float bottom);
//
inline glm::vec4 plane_from_quad(float left, float right, float top, float bottom);

bool against_quad(Ray const &ray, glm::vec4 const &bounds);

bool volume_intersection(Ray mins, Ray maxs, glm::vec4 quad);

template <> struct fmt::formatter<Ray> 
{
    constexpr auto parse(fmt::format_parse_context &ctx) -> decltype(ctx.begin()) 
    {
            return ctx.end();
    }

    template <typename FormatContext>
    auto format(const Ray &ray, FormatContext &ctx) const -> decltype(ctx.out()) 
    {
        return fmt::format_to(ctx.out(),
            "origin: {}\ndirection: {}",
            ray.origin,
            ray.direction);
    }
};
