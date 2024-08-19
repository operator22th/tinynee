#pragma once
#include "../externals/glm/glm.hpp"
#include <random>
#include "texture.h"
using namespace glm;

//==========================================const===========================================//
const float INF = 1e9;
const vec3 RED(1, 0.1, 0.1);
const vec3 GREEN(0.1, 1, 0.1);
const vec3 BLUE(0.1, 0.1, 1);
const vec3 YELLOW(1.0, 1.0, 0.1);
const vec3 CYAN(0.1, 1.0, 1.0);
const vec3 MAGENTA(1.0, 0.1, 1.0);
const vec3 GRAY(0.5, 0.5, 0.5);
const vec3 WHITE(1, 1, 1);
const float PI = 3.14159265359;
const float PI_MUL_2 = 2.0f * PI;
const float PI_MUL_4 = 4.0f * PI;
const float PI_DIV_2 = 0.5f * PI;
const float PI_DIV_4 = 0.25f * PI;
const float PI_INV = 1.0f / PI;
const float PI_MUL_2_INV = 1.0f / PI_MUL_2;
const float PI_MUL_4_INV = 1.0f / PI_MUL_4;
constexpr float EPS = 1e-9f;
constexpr float RAY_EPS = 1e-3f;

//==========================================random==========================================//

// 0-1 随机数生成
std::uniform_real_distribution<> dis(0.0, 1.0);
std::random_device rd;
std::mt19937 gen(rd());

double randf() {
    return dis(gen);
}

vec3 randomVec3() {
    vec3 d;
    do {
        d = 2.0f * vec3(randf(), randf(), randf()) - vec3(1, 1, 1);
    } while (dot(d, d) > 1.0);
    return normalize(d);
}

// one week
vec3 randomDirection(vec3 n) {
    // 法向球
    return normalize(randomVec3() + n);
}

// cos weighted
vec3 randomDirectionCosWeighted(vec3 n) {
    float r = randf();
    float theta = 2 * PI * randf();
    return normalize(vec3(sqrt(r) * cos(theta), sqrt(r) * sin(theta), sqrt(1 - r)));
}

//==========================================brdf==========================================//

// 物体表面材质定义
class Material {
public:
    vec3 color = vec3(0, 0, 0);     // 颜色
    bool isEmissive = false;        // 是否发光
    vec3 erate = vec3(3.0f, 3.0f, 3.0f); // 发光强度
    vec3 normal = vec3(0, 0, 0);    // 法向量
    double specularRate = 0.0f;     // 反射光占比
    double roughness = 0.5f;        // 粗糙程度
    double refractRate = 0.0f;      // 折射光占比
    double refractRatio = 1.0f;     // 折射率
    double refractRoughness = 0.0f; // 折射粗糙度
    float metallic = 0.0f;          // 金属度
    float specular = 0.5f;          // 镜面反射
    float specularTint = 0.0f;      // 镜面反射色调
    float sheen = 0.0f;             // 织物反射
    float sheenTint = 0.5f;         // 织物反射色调
    float clearcoat = 0.0f;         // 清漆
    float clearcoatGloss = 1.0f;    // 清漆光泽
    float subsurface = 0.0;
    Texture texture;
    Texture normalMap;
    Material(vec3 C){
        color = C;
    }
    Material() {}
} ;

enum BRDFType {
    DIFF,
    REFR,
    GLOSSY,
    TRANSL
};

//==========================================data==========================================//

// 光线求交结果
typedef struct HitResult {
    bool isHit = false;             // 是否命中
    float distance = 1e9; // 与交点的距离
    vec3 hitPoint = vec3(0, 0, 0);  // 光线命中点
    Material material;              // 命中点的表面材质
    vec3 hitColor;                 // 纹理映射颜色 or 颜色
    float time; // for motion blur
} HitResult;

// 光线
class Ray {
public:
    vec3 startPoint = vec3(0, 0, 0);    // 起点
    vec3 direction = vec3(0, 0, 0);     // 方向
    float time; // for motion blur
    Ray(){};
    Ray(vec3 start, vec3 dir, float time = 0) : startPoint(start), direction(dir), time(time) {}
};

typedef struct LightSampleResult {
    vec3 origin;    // 起点
    vec3 color;     // 颜色
    float pdf;     // 概率密度
    vec3 normal;    // 法向量
    vec3 erate;    // 发光强度
} LightSampleResult;

//==========================================image==========================================//
int ToInteger(double x) {
    /* Clamp to [0,1] */
    if (x < 0.0) x = 0.0;
    else if (x > 1.0) x = 1.0;

    /* Apply gamma correction and convert to integer */
    return int(pow(x, 1 / 2.2) * 255 + .5);
}

//==========================================others==========================================//
vec3 sphericalToCartesian(float theta, float phi) {
    return vec3(std::cos(phi) * std::sin(theta), std::cos(theta),
                 std::sin(phi) * std::sin(theta));
}

float clamp(float x, float a, float b) {
    return std::max(a, std::min(b, x));
}

float max(float a, float b) {
    return a > b ? a : b;
}

float min(float a, float b) {
    return a < b ? a : b;
}

// sample point on the disk
vec2 sampleDisk(float R, float& pdf) {
    float u1 = randf();
    float u2 = randf();
    const float r = R * std::sqrt(u1);
    const float theta = PI_MUL_2 * u2;
    pdf = 1.0f / (R * R) * PI_INV;
    return vec2(r * std::cos(theta), r * std::sin(theta));
}
