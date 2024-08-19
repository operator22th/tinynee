//独立实现

#pragma once
#include "util.h"

class Shape {
public:
    Shape(BRDFType B = DIFF) {
        switch (B) {
            case DIFF:
                material.specularRate = 0.3;
                break;
            case REFR:
                material.specularRate = 1.0;
                material.metallic = 1.0;
                material.roughness = 0.0;
                break;
            case GLOSSY:
                material.specularRate = 0.9;
                material.metallic = 0.8;
                material.roughness = 0.2;
                break;
            case TRANSL:
                material.refractRatio = 1.5;
                material.specularRate = 0.3;
                material.refractRate = 0.95;
                break;
            default:
                material.specularRate = 0.3;
                break;
        }
    }
    virtual HitResult intersect(Ray ray) { return HitResult(); }
    Material material;
};

// depreciated
//class vertice{
//public:
//    vec3 p;
//    vec3 n;
//    int id;
//    vertice(){};
//};

// 三角形
class Triangle : public Shape {
public:
    Triangle() {}

    Triangle(vec3 P1, vec3 P2, vec3 P3, Material m, vec3 I = vec3(3.0f, 3.0f, 3.0f), BRDFType B = DIFF, int id1= 0,
             int id2= 0, int id3= 0,
             bool smooth= false)
    :Shape(B), id1(id1), id2(id2), id3(id3)
    {
        p1 = P1, p2 = P2, p3 = P3;
        center = (p1 + p2 + p3) / 3.0f;
        material = m;
        material.normal = normalize(cross(p2 - p1, p3 - p1));
        material.erate = vec3(I.x, I.y, I.z);
        smoothNormal = smooth;
    }

    vec3 p1, p2, p3;
    vec3 n1, n2, n3;
    int id1, id2 ,id3;
    vec3 I;
    vec3 center;
    bool smoothNormal;

    HitResult intersect(Ray ray) override {
        HitResult res;

        vec3 S = ray.startPoint;
        vec3 d = ray.direction;
        vec3 N = material.normal;
        bool isInside = false;
        if (dot(N, d) > 0.0f) {
            N = -N;
            isInside = true;
        }

        if (fabs(dot(N, d)) < 0.00001f) return res;

        float t = (dot(N, p1) - dot(S, N)) / dot(d, N);
        if (t < 0.0005f) return res;

        vec3 P = S + d * t;

        vec3 c1 = cross(p2 - p1, P - p1);
        vec3 c2 = cross(p3 - p2, P - p2);
        vec3 c3 = cross(p1 - p3, P - p3);
        vec3 n = material.normal;
        bool r1 = (dot(c1, N) > 0 && dot(c2, N) > 0 && dot(c3, N) > 0);
        bool r2 = (dot(c1, N) < 0 && dot(c2, N) < 0 && dot(c3, N) < 0);

        //重心坐标系插值法向量
        float EPSILON = 0.0001f;
        //计算重心坐标系的三个参数(u,v,w)
        float u = (-(P.x - p2.x) * (p3.y - p2.y) + (P.y - p2.y) * (p3.x - p2.x)) /
                  (-(p1.x - p2.x) * (p3.y - p2.y) + (p1.y - p2.y) * (p3.x - p2.x));
        float v = (-(P.x - p3.x) * (p1.y - p3.y) + (P.y - p3.y) * (p1.x - p3.x)) /
                  ((p2.x - p3.x) * (p1.y - p3.y) + (p2.y - p3.y) * (p1.x - p3.x));
        float w = 1.0f - u - v;

        if(r1 || r2) {
            res.isHit = true;
            res.distance = t;
            res.hitPoint = P;
            res.material = material;

            if(material.normalMap.pic){
                res.material.normal = normalize(material.normalMap.getColor(u, v) * 2.0f - vec3(1,1,1));
                if(isInside)
                    res.material.normal = -res.material.normal;
            }else{
                if(!smoothNormal)
                    res.material.normal = N;
                else {
                    vec3 Nsmooth = u * n1 + v * n2 + w * n3;
                    Nsmooth = normalize(Nsmooth);
                    res.material.normal = N;
                }
            }
        }

        if(material.texture.pic)
            res.hitColor = material.texture.getColor(u, v);
        else
            res.hitColor = material.color;

        return res;
    };

    // Light Sample for Next Event Estimation
    LightSampleResult sampleLight() const {
        float r1 = randf();
        float r2 = randf();
        LightSampleResult res;
        //利用重心坐标系的三角形随机均匀点采样
        res.origin = (1.0f - sqrt(r1)) * p1 + (sqrt(r1) * (1.0f - r2)) * p2 + (sqrt(r1) * r2) * p3;
        res.color = material.color;
        res.normal = material.normal;
        res.pdf = 1.0f / (0.5f * length(cross(p2 - p1, p3 - p1)));
        res.erate = material.erate;
        return res;
    }
};


// 球
class Sphere : public Shape {
public:
    Sphere() {}

    Sphere(vec3 o, double r, vec3 c, BRDFType B = DIFF,
           float t0 = 0, float t1 = 0, vec3 o_prime= vec3(0)): Shape(B){
        O1 = o;
        R = r;
        time0 = t0;
        time1 = t1;
        O_prime = o_prime;
        material.color = c;
    }

    vec3 O1;
    double R;
    float time0;
    float time1;
    vec3 O_prime;
    Material material;

    vec3 get_O(float time){
        if(time0 == time1) return O1;
        return O1 + (time - time0) / (time1 - time0) * (O_prime - O1);
    }

    HitResult intersect(Ray ray) {
        HitResult res;

        vec3 O = get_O(ray.time);

        vec3 S = ray.startPoint;
        vec3 d = ray.direction;

        float OS = length(O - S);
        float SH = dot(O - S, d);
        float OH = sqrt(pow(OS, 2) - pow(SH, 2));

        if (OH > R) return res;

        float PH = sqrt(pow(R, 2) - pow(OH, 2));

        float t1 = length(SH) - PH;
        float t2 = length(SH) + PH;
        float t = (t1 < 0) ? (t2) : (t1);
        vec3 P = S + t * d;

        if (fabs(t1) < 0.0005f || fabs(t2) < 0.0005f) return res;

        res.isHit = true;
        res.distance = t;
        res.hitPoint = P;
        res.material = material;
        res.material.normal = t1 > 0 ?(normalize(P - O)): -(normalize(P - O)); //from inside???

        res.time = ray.time;

        res.hitColor = material.color;

        return res;
    }
};