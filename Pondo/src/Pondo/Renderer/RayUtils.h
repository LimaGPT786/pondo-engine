#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

// -------------------------------------------------------
//  Ray helpers — picking and world/screen conversion
// -------------------------------------------------------

struct Ray { glm::vec3 origin, dir; };

static float RayAABB(const Ray& ray, const glm::vec3& mn, const glm::vec3& mx)
{
    float tmin = 0.0f, tmax = 1e30f;
    for (int i = 0; i < 3; ++i) {
        if (glm::abs(ray.dir[i]) < 1e-8f) {
            if (ray.origin[i] < mn[i] || ray.origin[i] > mx[i]) return -1.0f;
            continue;
        }
        float invD = 1.0f / ray.dir[i];
        float t0 = (mn[i] - ray.origin[i]) * invD;
        float t1 = (mx[i] - ray.origin[i]) * invD;
        if (invD < 0.0f) std::swap(t0, t1);
        tmin = std::max(tmin, t0);
        tmax = std::min(tmax, t1);
        if (tmax < tmin) return -1.0f;
    }
    return tmin;
}

static Ray ScreenToRay(const glm::vec2& px, const glm::vec2& vpPos,
    const glm::vec2& vpSize, const glm::mat4& invVP)
{
    float nx = ((px.x - vpPos.x) / vpSize.x) * 2.0f - 1.0f;
    float ny = 1.0f - ((px.y - vpPos.y) / vpSize.y) * 2.0f;
    glm::vec4 n = invVP * glm::vec4(nx, ny, -1.0f, 1.0f); n /= n.w;
    glm::vec4 f = invVP * glm::vec4(nx, ny, 1.0f, 1.0f); f /= f.w;
    return { glm::vec3(n), glm::normalize(glm::vec3(f) - glm::vec3(n)) };
}

static glm::vec2 WorldToScreen(const glm::vec3& world, const glm::mat4& vp,
    const glm::vec2& vpPos, const glm::vec2& vpSize)
{
    glm::vec4 clip = vp * glm::vec4(world, 1.0f);
    if (glm::abs(clip.w) < 1e-6f) return { -9999, -9999 };
    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    return {
        vpPos.x + (ndc.x * 0.5f + 0.5f) * vpSize.x,
        vpPos.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * vpSize.y
    };
}