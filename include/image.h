#pragma once
#include <iostream>
#include <cmath>
#include "../externals/svpng.inc"
#include "util.h"
#include "../externals/glm/glm.hpp"
using namespace glm;

void savepng(double *SRC, int width, int height, const char *filename) {
    /* Save image in PNG format */
    unsigned char *image = new unsigned char[width * height * 3];
    unsigned char *p = image;
    double *S = SRC;

    FILE *fp;
    fp = fopen(filename, "wb");

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            *p++ = (unsigned char) clamp(pow(*S++, 1.0f / 2.2f) * 255, 0.0, 255.0);  // R 通道
            *p++ = (unsigned char) clamp(pow(*S++, 1.0f / 2.2f) * 255, 0.0, 255.0);  // G 通道
            *p++ = (unsigned char) clamp(pow(*S++, 1.0f / 2.2f) * 255, 0.0, 255.0);  // B 通道
        }
    }

    svpng(fp, width, height, image, 0);
}

void saveppm(double *S, int width, int height, const char *filename) {
    /* Save image in PPM format */
    FILE *f = fopen(filename, "wb");
    fprintf(f, "P3\n%d %d\n%d\n", width, height, 255);
    for (int i = 0; i < 3* height * width; i++)
        fprintf(f, "%d ", ToInteger(*S++));
    fclose(f);
}