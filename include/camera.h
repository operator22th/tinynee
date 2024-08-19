// 独立实现

#pragma once
#include "../externals/glm/glm.hpp"
#include "util.h"

class Camera {
protected:
    vec3 position;
    vec3 forward;
    vec3 right;
    vec3 up;
    float time0; // for motion blur 开始时间
    float time1; // 结束时间

public:
    Camera(const vec3& position, const vec3& forward, float t0, float t1): position(position), forward(forward) {
        right = normalize(cross(forward, vec3(0, 1, 0)));
        up = normalize(cross(right, forward));
        time0 = t0;
        time1 = t1;
    }

    virtual bool castRay(const vec2& uv, Ray& ray) const = 0;
};

class PinholeCamera : public Camera {
public:
    float focalLength;


    PinholeCamera(const vec3& position, const vec3& forward = vec3(0,0,-1), float focalLength = 2.9f, float FOV = 0.0f,
             float t0 = 0, float t1 = 0) : Camera(position, forward, t0, t1) {
        if(FOV == 0.0f) {
            this->focalLength = focalLength;
        } else {
            this->focalLength = 1.0f / std::tan(0.5f * FOV);
        }
    }

    bool castRay(const vec2& uv, Ray& ray) const override {
        float t = time0 + randf() * (time1 - time0);
        const vec3 pinholePos = position + focalLength * forward;
        const vec3 sensorPos = position + uv[0] * right + uv[1] * up;
        ray = Ray(sensorPos, normalize(pinholePos - sensorPos), t);
        return true;
    }
};

class SimpleCamera : public PinholeCamera {
public:

    SimpleCamera(const vec3& position, const vec3& forward = vec3(0,0,-1), float focalLength = 2.9f,
                 float t0 = 0, float t1 = 0): PinholeCamera(position, forward, focalLength, 0.0f, t0, t1) {
        this->position = position - forward * focalLength;
    }
};

// thin lens camera
class ThinLensCamera : public Camera {
private:
    float focalLength;
    float lensRadius;
    float a;    // distance from eye to the lens
    float b;     // distance from the lens to the object plane

public:
    ThinLensCamera(const vec3& position, const vec3& forward = vec3(0,0,-1), float focalLength = 2.9f, float FOV = 0.5f * PI,
                   float fNumber = 22.0f, float t0 = 0, float t1 = 0): Camera(position, forward, t0, t1) {
        // compute focal length from FOV
        if(FOV == 0.0f) {
            this->focalLength = focalLength;
        } else {
            this->focalLength = 1.0f / std::tan(0.5f * FOV);
        }
        // compute lens radius from F-number 光圈数
        lensRadius = 2.0f * focalLength / fNumber;

        b = 10000.0f; //物距
        a = 1.0f / (1.0f / focalLength - 1.0f / b); // 像距 1/u + 1/v = 1/f, u = a, v = b
    }

    // focus at specified position
    void focus(const vec3& p) {
        // b should be much larger than a for a normal camera
        // with means a may change little when b changes
        // So, we can calculate this way
        b = dot(p - position, forward) - a; // position is where the eye is
        a = 1.0f / (1.0f / focalLength - 1.0f / b);
    }

    bool castRay(const vec2& uv, Ray& ray) const override {
        float t = time0 + randf() * (time1 - time0);

        const vec3 sensorPos = position + uv[0] * right + uv[1] * up;
        const vec3 lensCenter = position + a * forward;

        // sample point on lens
        float pdf_area;
        const vec2 pLens2D = sampleDisk(lensRadius, pdf_area);
        const vec3 pLens = lensCenter + pLens2D[0] * right + pLens2D[1] * up;
        vec3 sensorToLens = normalize(pLens - sensorPos);

        // find intersection point with object plane
        const vec3 sensorToLensCenter = normalize(lensCenter - sensorPos);
        const vec3 pObject = sensorPos + ((a + b) / dot(sensorToLensCenter, forward)) * sensorToLensCenter;

        ray = Ray(pLens, normalize(pObject - pLens), t);
        return true;
    }

};
