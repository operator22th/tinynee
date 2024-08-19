//独立实现

#pragma once
#include "shape.h"
#include "bvh.h"
#include "texture.h"
#include <bits/stdc++.h>
#include "../externals/glm/gtc/matrix_transform.hpp"
#include "../externals/tiny_obj_loader.h"
#include <iostream>

class Mesh: public Shape {
public:
    Mesh() {

    }

    std::vector <vec3> vertices;
    std::vector <vec3> n;
    std::vector <int> indices;
    glm::mat4 trans;

    std::vector<Triangle> t;
    BVHNode* root;
    Material material;
    bool bruteForce = false;

    // 模型变换矩阵
    mat4 getTransformMatrix(vec3 rotateCtrl, vec3 translateCtrl, vec3 scaleCtrl) {
        glm::mat4 unit( // 单位矩阵
                glm::vec4(1, 0, 0, 0),
                glm::vec4(0, 1, 0, 0),
                glm::vec4(0, 0, 1, 0),
                glm::vec4(0, 0, 0, 1)
        );
        mat4 scale = glm::scale(unit, scaleCtrl);
        mat4 translate = glm::translate(unit, translateCtrl);
        mat4 rotate = unit;
        rotate = glm::rotate(rotate, glm::radians(rotateCtrl.x), glm::vec3(1, 0, 0));
        rotate = glm::rotate(rotate, glm::radians(rotateCtrl.y), glm::vec3(0, 1, 0));
        rotate = glm::rotate(rotate, glm::radians(rotateCtrl.z), glm::vec3(0, 0, 1));
        mat4 model = translate * rotate * scale;

        return model;
    }
    
    struct TriangleIndex {
        TriangleIndex() {
            x[0] = 0;
            x[1] = 0;
            x[2] = 0;
        }

        int &operator[](const int i) { return x[i]; }

        // By Computer Graphics convention, counterclockwise winding is front face
        int x[3]{};
    };

    HitResult intersect(Ray ray) {
        if(bruteForce){
            HitResult res;
            for (int i = 0; i < (int) t.size(); ++i) {
                HitResult r = t[i].intersect(ray);
                if(r.isHit && r.distance < res.distance) {
                    res = r;
                }
            }
            return res;
        }
        else
            return hitBVH(ray, t, root);
    }

    Mesh(const char *filename, vec3 c, vec3 rotateCtrl, vec3 translateCtrl, vec3 scaleCtrl,
            bool bruteForce = false, bool smooth=false, const char* texturefile="", const char* normfile="") : bruteForce(bruteForce) {
        trans = getTransformMatrix(rotateCtrl, translateCtrl, scaleCtrl);
        std::ifstream f;
        f.open(filename);
        if (!f.is_open()) {
            std::cout << "Cannot open " << filename << "\n";
            return;
        }
        std::string line;
        std::string vTok("v");
        std::string fTok("f");
        std::string texTok("vt");
        char bslash = '/', space = ' ';
        std::string tok;
        int texID;

        float maxx = -INF;
        float maxy = -INF;
        float maxz = -INF;
        float minx = INF;
        float miny = INF;
        float minz = INF;

        Material m = Material();
        m.color = c;

        if(strlen(texturefile) > 0){
            m.texture = Texture(texturefile);
        }
        if(strlen(normfile) > 0){
            m.normalMap = Texture(normfile);
        }

        while (true) {
            std::getline(f, line);
            if (f.eof()) {
                break;
            }
            if (line.size() < 3) {
                continue;
            }
            if (line.at(0) == 'f') {
                break;
            }
            if (line.at(0) == '#') {
                continue;
            }
            std::stringstream ss(line);
            ss >> tok;
            if (tok == vTok) {
                // 读取顶点
                vec3 vec;
                ss >> vec[0] >> vec[1] >> vec[2];
                maxx = max(maxx, vec[0]);
                maxy = max(maxx, vec[1]);
                maxz = max(maxx, vec[2]);
                minx = min(minx, vec[0]);
                miny = min(minx, vec[1]);
                minz = min(minx, vec[2]);
                vertices.push_back(vec);
                if(smooth)
                    n.push_back(vec3(0, 0, 0));
            }
        }

        // 模型大小归一化
        float lenx = maxx - minx;
        float leny = maxy - miny;
        float lenz = maxz - minz;
        float maxaxis = max(lenx, max(leny, lenz));
        for (auto& vt : vertices) {
            vt.x /= maxaxis;
            vt.y /= maxaxis;
            vt.z /= maxaxis;
        }

        // 通过矩阵进行坐标变换
        for (auto& vt : vertices) {
            vec4 vv = vec4(vt.x, vt.y, vt.z, 1);
            vv = trans * vv;
            vt = vec3(vv.x, vv.y, vv.z);
        }

        while(true){
            std::getline(f, line);
            if (f.eof()) {
                break;
            }
            if (line.at(0) == '#') {
                continue;
            }
            std::stringstream ss(line);
            ss >> tok;

            if (tok == vTok) {
                exit(-1);
            }
            else if (tok == fTok) {
                // 读取面
                if (line.find(bslash) != std::string::npos) { // 有纹理坐标
                    std::replace(line.begin(), line.end(), bslash, space);
                    std::stringstream facess(line);
                    TriangleIndex trig;
                    facess >> tok;
                    for (int ii = 0; ii < 3; ii++) {
                        facess >> trig[ii] >> texID;
                        trig[ii]--;
                    }
                    if(smooth) {
                        vec3 norm = normalize(
                                cross(vertices[trig[1]] - vertices[trig[0]], vertices[trig[2]] - vertices[trig[0]]));
                        n[trig[0]] += norm;
                        n[trig[1]] += norm;
                        n[trig[2]] += norm;

                        t.push_back(Triangle(vertices[trig[0]], vertices[trig[1]], vertices[trig[2]], m, vec3(3.0f, 3.0f, 3.0f), DIFF,
                                             trig[0], trig[1], trig[2], smooth));
                    }
                    else{
                        t.push_back(Triangle(vertices[trig[0]], vertices[trig[1]], vertices[trig[2]], m));
                    }
                } else {
                    TriangleIndex trig;
                    for (int ii = 0; ii < 3; ii++) {
                        t.push_back(Triangle(vertices[trig[0]], vertices[trig[1]], vertices[trig[2]], m));
                    }
                }
            } else if (tok == texTok) { // 纹理坐标
                vec2 texcoord;
                ss >> texcoord[0];
                ss >> texcoord[1];
            }
        }

        if(smooth) {
            for (auto &norm: n) {
                norm = normalize(norm);
            }

            for (auto &tri: t) {
                tri.n1 = n[tri.id1];
                tri.n2 = n[tri.id2];
                tri.n3 = n[tri.id3];
            }
        }

        material.color = c;
        if(bruteForce)
            root = nullptr;
        else
            root = buildBVH(t, 0, t.size() - 1, 8);
        f.close();
    }
};
