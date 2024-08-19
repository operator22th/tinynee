//基于smallpt独立实现

#pragma once
#include <vector>
#include <cstring>
#include <omp.h>
#include "image.h"
#include "shape.h"
#include "material.h"
#include "scene.h"
#include "camera.h"

using namespace std;

HitResult shoot(vector<Shape *> &shapes, Ray ray) {
    HitResult res, r;
    for (auto &shape: shapes) {
        r = shape->intersect(ray);
        if (r.isHit && r.distance < res.distance) res = r;
    }
    return res;
}

class SimpleRenderer{
public:

    vec3 pathTracingNEE(vector<Shape *> &shapes, Ray ray, int depth, vector<Triangle *> &lights, Material& material, vec3 hitColor,
                        bool legacy = true, vec3 indirect = vec3(0)) {

        vec3 color = vec3(0);
        vec3 normal = material.normal;

        if(!legacy && indirect == vec3(0)) {
            exit(-1);
        }

        // 对光源采样
        for (auto &light: lights) {
            LightSampleResult lsr = light->sampleLight();
            vec3 L = lsr.origin - ray.startPoint;
            double distance = length(L);

            Ray shadowRay;
            shadowRay.startPoint = ray.startPoint;
            shadowRay.direction = normalize(L);
            shadowRay.time = ray.time;


            HitResult shadowRes = shoot(shapes, shadowRay);
            if (abs(shadowRes.distance - distance) < 0.01f && shadowRes.material.isEmissive) {
                float cosTheta = dot(shadowRay.direction, normal);
                float cosTheta2 = dot(shadowRay.direction, lsr.normal);
                if (cosTheta * cosTheta2 > 0) {
                    float G = cosTheta * cosTheta2 / (distance * distance);
                    float pdf = lsr.pdf;
                    float weight = 1.0 / pdf;
                    vec3 fr;
                    if(legacy) {
                        // for legacy method, we think every thing as
                        // a mix of ambient and reflect(except reflect part)
                        // we use where the ray goes to simulate the material
                        // and we think every thing goes on well with Lambert
                        fr = hitColor * PI_INV;
                    } else {
                        fr = BRDF_Evaluate(-indirect, material.normal, shadowRay.direction, material, hitColor);
                    }

                    color += fr * G * weight * lsr.erate;
                }
            }
        }

        if (depth > 10) return vec3(0);
        // 普通采样
        HitResult res = shoot(shapes, ray);

        if (!res.isHit) return vec3(0);

        if (res.material.isEmissive) return color; //直接光照已经算过了，直接返回就行

        double r = randf();

        // from smallpt
        vec3 f = res.hitColor;
        // use maximum reflectivity amount of Russian roulette
        float P = f.x > f.y && f.x > f.z ? f.x : f.y > f.z ? f.y : f.z; // max refl
//        P = max(P - 0.1f, 0.1f);  // P range from 0.1 to 0.9

        if (depth > 4) {
            if (r >= P)
                return vec3(0);
        }

        Ray nextRay;
        nextRay.startPoint = res.hitPoint;
        nextRay.direction = randomDirection(res.material.normal);
        nextRay.time = ray.time;

        float cosine = fabs(dot(-ray.direction, res.material.normal));

        r = randf();
        if(!res.material.isEmissive) {
            if(legacy) {
                if (r < res.material.specularRate) {
                    vec3 ref = normalize(reflect(ray.direction, res.material.normal));
                    nextRay.direction = mix(ref, nextRay.direction, res.material.roughness);
                    color += pathTracingNEE(shapes, nextRay, depth + 1, lights, res.material, res.hitColor) * cosine / P;
                } else if (res.material.specularRate <= r && r <= res.material.refractRate) {
                    vec3 ref = normalize(refract(ray.direction, res.material.normal, float(res.material.refractRate)));
                    nextRay.direction = mix(ref, -nextRay.direction, res.material.refractRoughness);
                    color += pathTracingNEE(shapes, nextRay, depth + 1, lights, res.material, res.hitColor) * cosine / P;
                } else {
                    vec3 srcColor = res.hitColor;
                    vec3 ptColor = pathTracingNEE(shapes, nextRay, depth + 1, lights, res.material, res.hitColor) * cosine;
                    color += ptColor * srcColor / P;
                }
            }
            else{
                //TODO:更加合理的importance sampling；目前这个没有按照brdf的概率来采样；但效果还行

                if (r < res.material.specularRate) {
                    vec3 ref = normalize(reflect(ray.direction, res.material.normal));
                    nextRay.direction = mix(ref, nextRay.direction, res.material.roughness);
                    color += pathTracingNEE(shapes, nextRay, depth + 1, lights, res.material, res.hitColor, legacy, ray.direction) * cosine / P;
                } else if (res.material.specularRate <= r && r <= res.material.refractRate) {
                    vec3 ref = normalize(refract(ray.direction, res.material.normal, float(res.material.refractRate)));
                    nextRay.direction = mix(ref, -nextRay.direction, res.material.refractRoughness);
                    color += pathTracingNEE(shapes, nextRay, depth + 1, lights, res.material, res.hitColor, legacy, ray.direction) * cosine / P;
                } else {
                    vec3 srcColor = res.hitColor;
                    vec3 ptColor = pathTracingNEE(shapes, nextRay, depth + 1, lights, res.material, res.hitColor, legacy, ray.direction) * cosine;
                    color += ptColor * srcColor / P;
                }
            }
        }

        return color;
    }

    void render(EasyScene& scene, Camera& camera, int width, int height, int samples, const std::string &filename, bool legacy = true) {
        vector<Shape *>shapes = scene.shapes;
        vector<Triangle *>lights = scene.lights;
        double *image = new double[width * height * 3];
        memset(image, 0.0, sizeof(double) * width * height * 3);

        omp_set_num_threads(50);
#pragma omp parallel for
        for (int k = 0; k < samples; k++) {
            for (int sx = 0; sx < 2; ++sx) {
                for (int sy = 0; sy < 2; ++sy) {
                    double *p = image;

                    for (int i = 0; i < height; i++) {
                        fprintf(stderr, "\rRendering (%d spp)", samples * 4);
                        for (int j = 0; j < width; j++) {

                            double x = 2.0 * double(j) / double(width) - 1.0;
                            double y = 2.0 * double(i) / double(height) - 1.0;

                            //tent filter
                            double r1 = 2.0 * randf();
                            r1 = r1 < 1 ? sqrt(r1) - 1 : 1 - sqrt(2 - r1);
                            double r2 = 2.0 * randf();
                            r2 = r2 < 1 ? sqrt(r2) - 1 : 1 - sqrt(2 - r2);

                            //多重采样抗锯齿
                            x += (sx + 0.5 + r1) / double(width);
                            y -= (sy + 0.5 + r2) / double(height);;
//
                            Ray ray;
                            camera.castRay(vec2(x, y), ray);

                            // 与场景的交点
                            HitResult res = shoot(shapes, ray);
                            vec3 color = vec3(0, 0, 0);

                            if (res.isHit) {
                                if (res.material.isEmissive) {
                                    color = res.hitColor;
                                }
                                else {
                                    Ray nextRay;
                                    nextRay.startPoint = res.hitPoint;
                                    nextRay.direction = randomDirection(res.material.normal);
                                    nextRay.time = ray.time;

                                    double r = randf();
                                    if (r < res.material.specularRate) {
                                        vec3 ref = normalize(reflect(ray.direction, res.material.normal));
                                        nextRay.direction = mix(ref, nextRay.direction, res.material.roughness);
                                        color = pathTracingNEE(shapes, nextRay, 0, lights, res.material, res.hitColor, legacy, ray.direction);
                                    }
                                    else if (res.material.specularRate <= r && r <= res.material.refractRate) {
                                        vec3 ref = normalize(
                                                refract(ray.direction, res.material.normal,
                                                        float(res.material.refractRate)));
                                        nextRay.direction = mix(ref, -nextRay.direction, res.material.refractRoughness);
                                        color = pathTracingNEE(shapes, nextRay, 0, lights, res.material, res.hitColor, legacy, ray.direction);
                                    }
                                    else {
                                        vec3 srcColor = res.hitColor;
                                        vec3 ptColor = pathTracingNEE(shapes, nextRay, 0, lights, res.material, res.hitColor, legacy, ray.direction);
                                        color = ptColor * srcColor;
                                    }

                                    //1/pdf is always 2*PI thanks to our living in a 3D world
                                    color *= (2.0f * 3.1415926f) * (1.0f / double(samples)) * 0.25;
                                }
                            }

                            *p += color.x;
                            p++;
                            *p += color.y;
                            p++;
                            *p += color.z;
                            p++;
                        }
                    }
                }
            }
        }
        if(filename.find(".png") != string::npos)
            savepng(image, width, height, filename.c_str());
        else
            saveppm(image, width, height, filename.c_str());
        printf("\nSaved image to %s\n", filename.c_str());
    }
};
