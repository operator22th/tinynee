//从texture里读取对应color的这部分代码 参考了往届学长代码实现 https://github.com/Guangxuan-Xiao/THU-Computer-Graphics-2020
//其他和纹理相关的代码独立实现

#pragma once
#include "../externals/glm/glm.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "../externals/stb_image.h"
#include <cstring>
using namespace glm;

// Fixme: change it

class Texture {  // 纹理
public:
    unsigned char *pic;
    int w, h, c;

    Texture() : pic(nullptr), w(0), h(0), c(0) {}

    Texture(const char *textureFile){
        if (strlen(textureFile) > 0) {
            pic = stbi_load(textureFile, &w, &h, &c, 0);
            printf("Texture file: %s loaded. Size: %dx%dx%d\n", textureFile, w, h,
                   c);
        } else {
            pic = nullptr;
            printf("Texture file: %s not found.\n", textureFile);
            exit(-1);
        }
    }

    vec3 getColor(float u, float v) const{
        if (!pic) return vec3(0);

        u -= int(u);
        v -= int(v);
        u = u < 0 ? 1 + u : u;
        v = v < 0 ? 1 + v : v;
        u = u * w;
        v = h * (1 - v);
        int iu = (int)u, iv = (int)v;
        float u_prime = u - iu, v_prime = v - iv;
        vec3 c(0);

        c += (1 - u_prime) * (1 - v_prime) * getPixel(iu, iv);
        c += u_prime * (1 - v_prime) * getPixel(iu + 1, iv);
        c += (1 - u_prime) * v_prime * getPixel(iu, iv + 1);
        c += u_prime * v_prime * getPixel(iu + 1, iv + 1);

        c.x = clamp(c.x, 0.0f, 1.0f);
        c.y = clamp(c.y, 0.0f, 1.0f);
        c.z = clamp(c.z, 0.0f, 1.0f);
        return c;
    }

    vec3 getPixel(int u, int v) const{
        if (!pic) return vec3(0);
        u = clamp(u, 0, w - 1);
        v = clamp(v, 0, h - 1);
        int idx = v * w * c + u * c;
        return vec3(pic[idx + 0], pic[idx + 1], pic[idx + 2]) / 255.0f;
    }
};
