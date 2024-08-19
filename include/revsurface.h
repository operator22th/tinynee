//implemented on the base of https://github.com/Guangxuan-Xiao/THU-Computer-Graphics-2020

#pragma once
#include "../externals/glm/gtc/matrix_transform.hpp"
#include "bvh.h"
#include "curve.h"
#include "shape.h"

const int NEWTON_STEPS = 20;
const float NEWTON_EPS = 1e-2;
class RevSurface : public Shape {
    BezierCurve *pCurve;
    vec3 aa;
    vec3 bb;
    std::vector<Triangle> triangles;
    Material material;
   public:
    RevSurface(BezierCurve *pCurve, Material m)
        : pCurve(pCurve) {
        // Check flat.
        material = m;
        for (const auto &cp : pCurve->getControls()) {
            if (cp.z != 0.0) {
                printf("Profile of revSurface must be flat on xy plane.\n");
                exit(0);
            }
        }
        aa = vec3(-pCurve->radius, pCurve->ymin - 3, -pCurve->radius);
        bb = vec3(pCurve->radius, pCurve->ymax + 3, pCurve->radius);
    }

    HitResult intersect(const Ray r) override {
        float t, rou, mu;
        HitResult rst;
        //AABB进行加速
        if (hitAABB(r, aa, bb) == -1) return rst;

        vec3 normal, point;

        vec3 dmu, drou;

        int i;
        for (i = 0; i < NEWTON_STEPS; ++i) {
            // Newton's method
            // 求解核心
            if (rou < 0.0) rou += 2 * PI;
            if (rou >= 2 * PI) rou = fmod(rou, 2 * PI);

            if (mu <= 0) mu = FLT_EPSILON;
            if (mu >= 1) mu = 1.0 - FLT_EPSILON;

            point = getPoint(rou, mu, drou, dmu);

            vec3 f = r.startPoint + r.direction * t - point;
            float dist = glm::length(f);
            normal = cross(dmu, drou);
            if (dist < NEWTON_EPS) break;

            float D = dot(r.direction, normal);
            // 迭代
            t -= dot(dmu, cross(drou, f)) / D;
            mu -= dot(r.direction, cross(drou, f)) / D;
            rou += dot(r.direction, cross(dmu, f)) / D;
        }

        // out of steps
        if(i == NEWTON_STEPS || !isnormal(mu) || !isnormal(rou) || !isnormal(t)) return rst;

        if (t < 0 || mu < 0 || mu > 1)
            return rst;

        if (dot(normal, r.direction) > 0.0f) {
            normal = -normal;
        }

        rst.isHit = true;
        rst.distance = t;
        rst.hitPoint = r.startPoint + r.direction * t;
        rst.material = material;
        rst.time = r.time;
        rst.hitColor = material.color;
        rst.material.normal = normalize(normal);
        rst.hitColor = material.color;

        return rst;
    }

    vec3 getPoint(const float &rou, const float &mu, vec3 &drou, vec3 &dmu) {
        vec3 pt;
        glm::mat4 unit( // 单位矩阵
                glm::vec4(1, 0, 0, 0),
                glm::vec4(0, 1, 0, 0),
                glm::vec4(0, 0, 1, 0),
                glm::vec4(0, 0, 0, 1)
        );
        mat4 trans = unit;
        trans = glm::rotate(trans, rou, vec3(0,1,0));
        CurvePoint cp = pCurve->getPoint(mu);
        vec4 p= trans * vec4(cp.V, 1);
        pt = vec3(p.x, p.y, p.z);
        vec4 d = trans * vec4(cp.T, 1);
        dmu = vec3(d.x, d.y, d.z);
        drou = vec3(-cp.V.x * sin(rou), 0, -cp.V.x * cos(rou));
        return pt;
    }
};
