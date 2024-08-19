#pragma once
#include "util.h"
#include "camera.h"
#include "shape.h"
#include <unordered_map>
#include <optional>
#include <string>
#include <memory>
#include <iostream>
#include <filesystem>
#include "revsurface.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "../externals/tiny_obj_loader.h"
#include "mesh.h"

class EasyScene {
public:
    std::vector<Shape*> shapes;
    std::vector<Triangle*> lights;
    std::vector<float> texcoords; //TODO

    void LoadScene(const std::string &filename) {
        std::vector<tinyobj::shape_t> _shapes;
        std::vector<tinyobj::material_t> _materials;

        std::cout << "Loading " << filename << "..." << std::endl;

        // Load OBJ elements with tinyOBJLoader
        std::string err;
        bool ret = tinyobj::LoadObj(_shapes, _materials, err, filename.c_str(),"");

        if (!err.empty()) {
            std::cerr << err << std::endl;
        }

        if (!ret) {
            exit(1);
        }

        std::cout << "# of objects: " << _shapes.size() << std::endl;
        std::cout << "# of materials: " << _materials.size() << std::endl;

        // Extract elements and build triangles for our scene from them
        std::vector<vec3> vertices;
        for (size_t i = 0; i < _shapes.size(); i++) {
            // Collect the vertices for this object
            for (size_t v = 0; v < _shapes[i].mesh.positions.size() / 3; v++) {
                vertices.push_back(
                        vec3(_shapes[i].mesh.positions[3 * v + 0],
                             _shapes[i].mesh.positions[3 * v + 1],
                             _shapes[i].mesh.positions[3 * v + 2])
                );
            }

            // Determine color of object
            // TODO: diffuse reflect and ambient reflect color may differ
            int type = 2;
            Material m;
            vec3 triColor;

            if(!_materials.empty()) {
                triColor = vec3(_materials[_shapes[i].mesh.material_ids[0]].diffuse[0],
                                     _materials[_shapes[i].mesh.material_ids[0]].diffuse[1],
                                     _materials[_shapes[i].mesh.material_ids[0]].diffuse[2]);
                type = _materials[_shapes[i].mesh.material_ids[0]].illum;
            }
            else {
                printf("Fatal!!! No material specified for a scene!!!");
                exit(0);
            }
            m.color = triColor;
            // Check if it's supposed to be a lightsource
            vec3 lightIntensity = vec3(0.0f);
            BRDFType brdfType;
            switch (type) {
                case 0:
                    //TODO
                    break;
                case 1:
                    //TODO
                    break;
                case 2:
                    brdfType = DIFF;
                    break;
                case 3:
                    //TODO
                    break;
                case 4:
                    brdfType = GLOSSY;
                    break;
                case 5:
                    brdfType = REFR;
                    break;
                case 7:
                    brdfType = TRANSL;
                    break;
                default:
                    brdfType = DIFF;
                    break;
            }

            if (_materials[_shapes[i].mesh.material_ids[0]].emission[0] || _materials[_shapes[i].mesh.material_ids[0]].emission[1]
                || _materials[_shapes[i].mesh.material_ids[0]].emission[2]) {
                lightIntensity[0] = _materials[_shapes[i].mesh.material_ids[0]].emission[0];
                lightIntensity[1] = _materials[_shapes[i].mesh.material_ids[0]].emission[1];
                lightIntensity[2] = _materials[_shapes[i].mesh.material_ids[0]].emission[2];
            }

            for (size_t b = 0; b < _shapes[i].mesh.indices.size(); b += 3) {
                Triangle *newtri = new Triangle(vertices[_shapes[i].mesh.indices[b]],
                                                vertices[_shapes[i].mesh.indices[b + 1]],
                                                vertices[_shapes[i].mesh.indices[b + 2]],
                                                m, lightIntensity, brdfType);

                if(lightIntensity[0] > 0 || lightIntensity[1] > 0 || lightIntensity[2] > 0) {
                    newtri->material.isEmissive = true;
//                    newtri->material.normal = -newtri->material.normal;
                    lights.push_back(newtri);
                }

                shapes.push_back(newtri);
            }
            // Clear stack for next triangles
            vertices.clear();
        }
    }

    void loadObjScene(const char* filename, const char* textureFile="", const char* normFile="", bool bruteForce = false, bool smooth = false){
//        Mesh* m = new Mesh(filename, WHITE, vec3(0.8, 0.4, 0.1), vec3(0.0, 0.0, 0.0), vec3(0.5, 0.5, 0.5), bruteForce, smooth, textureFile, normFile);
        Mesh* m = new Mesh(filename, WHITE, vec3(0, 0, 0), vec3(0.3, -1.2, 0.0), vec3(1.0, 1.0, 1.0), bruteForce, smooth, textureFile, normFile);
        shapes.push_back(m);
        // 发光物
        Triangle* l1 = new Triangle(vec3(0.4, 0.99, 0.4), vec3(-0.4, 0.99, -0.4), vec3(-0.4, 0.99, 0.4), Material(WHITE));
        Triangle* l2 = new Triangle(vec3(0.4, 0.99, 0.4), vec3(0.4, 0.99, -0.4), vec3(-0.4, 0.99, -0.4), Material(WHITE));
        l1->material.isEmissive = true;
        l2->material.isEmissive = true;
        lights.push_back(l1);
        lights.push_back(l2);
        shapes.push_back(l1);
        shapes.push_back(l2);

        // 背景盒子
        // bottom
        shapes.push_back(new Triangle(vec3(1, -1, 1), vec3(-1, -1, -1), vec3(-1, -1, 1), Material(WHITE)));
        shapes.push_back(new Triangle(vec3(1, -1, 1), vec3(1, -1, -1), vec3(-1, -1, -1), Material(WHITE)));
        // top
        shapes.push_back(new Triangle(vec3(1, 1, 1), vec3(-1, 1, 1), vec3(-1, 1, -1), Material(WHITE)));
        shapes.push_back(new Triangle(vec3(1, 1, 1), vec3(-1, 1, -1), vec3(1, 1, -1), Material(WHITE)));
        // back
        shapes.push_back(new Triangle(vec3(1, -1, -1), vec3(-1, 1, -1), vec3(-1, -1, -1), Material(CYAN)));
        shapes.push_back(new Triangle(vec3(1, -1, -1), vec3(1, 1, -1), vec3(-1, 1, -1), Material(CYAN)));
        // left
        shapes.push_back(new Triangle(vec3(-1, -1, -1), vec3(-1, 1, 1), vec3(-1, -1, 1), Material(BLUE)));
        shapes.push_back(new Triangle(vec3(-1, -1, -1), vec3(-1, 1, -1), vec3(-1, 1, 1), Material(BLUE)));
        // right
        shapes.push_back(new Triangle(vec3(1, 1, 1), vec3(1, -1, -1), vec3(1, -1, 1), Material(RED)));
        shapes.push_back(new Triangle(vec3(1, -1, -1), vec3(1, 1, 1), vec3(1, 1, -1), Material(RED)));
    }

    void loadFinalScene(const char* filename, const char* textureFile="", const char* normFile="", bool bruteForce = false, bool smooth = false){
        Mesh* m = new Mesh(filename, WHITE, vec3(0, 0, 0), vec3(0.3, -1.2, 0.0), vec3(1.0, 1.0, 1.0), bruteForce, smooth, textureFile);
        shapes.push_back(m);

        // brdf
        Sphere* s2 = new Sphere(vec3(0.65, 0.1, 0.0), 0.3, GRAY, DIFF); // change the BRDF in nextfew lines
        s2->material.specularRate = 0.9;
        s2->material.roughness = 0.2;
        s2->material.metallic = 0.8;
        shapes.push_back(s2);

        // motion blur
        Sphere* s3 = new Sphere(vec3(-0.65, -0.7, 0.0), 0.05, WHITE, DIFF, 0, 1, vec3(-0.55, -0.6, 0.1));
        s3->material.specularRate = 0.95;
        s3->material.roughness = 0.1;
        s3->material.metallic = 0.9;
        shapes.push_back(s3);

        // 发光物
        Triangle* l1 = new Triangle(vec3(0.4, 0.99, 0.4), vec3(-0.4, 0.99, -0.4), vec3(-0.4, 0.99, 0.4), Material(WHITE));
        Triangle* l2 = new Triangle(vec3(0.4, 0.99, 0.4), vec3(0.4, 0.99, -0.4), vec3(-0.4, 0.99, -0.4), Material(WHITE));
        l1->material.isEmissive = true;
        l2->material.isEmissive = true;
        lights.push_back(l1);
        lights.push_back(l2);
        shapes.push_back(l1);
        shapes.push_back(l2);

        // 背景盒子
        // bottom
        shapes.push_back(new Triangle(vec3(1, -1, 1), vec3(-1, -1, -1), vec3(-1, -1, 1), Material(WHITE)));
        shapes.push_back(new Triangle(vec3(1, -1, 1), vec3(1, -1, -1), vec3(-1, -1, -1), Material(WHITE)));

        // top
        shapes.push_back(new Triangle(vec3(1, 1, 1), vec3(-1, 1, 1), vec3(-1, 1, -1), Material(WHITE)));
        shapes.push_back(new Triangle(vec3(1, 1, 1), vec3(-1, 1, -1), vec3(1, 1, -1), Material(WHITE)));

        // back
        auto mw = Material(CYAN);
        mw.normalMap = normFile;
        shapes.push_back(new Triangle(vec3(1, -1, -1), vec3(-1, 1, -1), vec3(-1, -1, -1), mw));
        mw.normalMap = "NormalMap2.png";
        shapes.push_back(new Triangle(vec3(-1, 1, -1),vec3(1, -1, -1), vec3(1, 1, -1),  mw));

        // left
        shapes.push_back(new Triangle(vec3(-1, -1, -1), vec3(-1, 1, 1), vec3(-1, -1, 1), Material(BLUE)));
        shapes.push_back(new Triangle(vec3(-1, -1, -1), vec3(-1, 1, -1), vec3(-1, 1, 1), Material(BLUE)));
        // right
        shapes.push_back(new Triangle(vec3(1, 1, 1), vec3(1, -1, -1), vec3(1, -1, 1), Material(RED)));
        shapes.push_back(new Triangle(vec3(1, -1, -1), vec3(1, 1, 1), vec3(1, 1, -1), Material(RED)));
    }

    void LoadDefaultScene(){
        // not brdf
        Sphere* s1 = new Sphere(vec3(-0.65, -0.7, 0.0), 0.3, WHITE);
        Sphere* s2 = new Sphere(vec3(0.0, -0.3, 0.0), 0.4, WHITE);
        Sphere* s3 = new Sphere(vec3(0.65, 0.1, 0.0), 0.3, BLUE);
        s1->material.specularRate = 1.0;
        s1->material.roughness = 0.0;

        s2->material.specularRate = 0.3;
        s2->material.refractRate = 0.95;
        s2->material.refractRatio = 0.1;

        s3->material.specularRate = 0.3;

        shapes.push_back(s1);
        shapes.push_back(s2);
        shapes.push_back(s3);

        // 发光物
        Triangle* l1 = new Triangle(vec3(0.4, 0.99, 0.4), vec3(-0.4, 0.99, -0.4), vec3(-0.4, 0.99, 0.4), Material(WHITE));
        Triangle* l2 = new Triangle(vec3(0.4, 0.99, 0.4), vec3(0.4, 0.99, -0.4), vec3(-0.4, 0.99, -0.4), Material(WHITE));
        l1->material.isEmissive = true;
        l2->material.isEmissive = true;
        lights.push_back(l1);
        lights.push_back(l2);
        shapes.push_back(l1);
        shapes.push_back(l2);

        // 背景盒子
        // bottom
        shapes.push_back(new Triangle(vec3(1, -1, 1), vec3(-1, -1, -1), vec3(-1, -1, 1), Material(WHITE)));
        shapes.push_back(new Triangle(vec3(1, -1, 1), vec3(1, -1, -1), vec3(-1, -1, -1), Material(WHITE)));
        // top
        shapes.push_back(new Triangle(vec3(1, 1, 1), vec3(-1, 1, 1), vec3(-1, 1, -1), Material(WHITE)));
        shapes.push_back(new Triangle(vec3(1, 1, 1), vec3(-1, 1, -1), vec3(1, 1, -1), Material(WHITE)));
        // back
        shapes.push_back(new Triangle(vec3(1, -1, -1), vec3(-1, 1, -1), vec3(-1, -1, -1), Material(CYAN)));
        shapes.push_back(new Triangle(vec3(1, -1, -1), vec3(1, 1, -1), vec3(-1, 1, -1), Material(CYAN)));
        // left
        shapes.push_back(new Triangle(vec3(-1, -1, -1), vec3(-1, 1, 1), vec3(-1, -1, 1), Material(BLUE)));
        shapes.push_back(new Triangle(vec3(-1, -1, -1), vec3(-1, 1, -1), vec3(-1, 1, 1), Material(BLUE)));
        // right
        shapes.push_back(new Triangle(vec3(1, 1, 1), vec3(1, -1, -1), vec3(1, -1, 1), Material(RED)));
        shapes.push_back(new Triangle(vec3(1, -1, -1), vec3(1, 1, 1), vec3(1, 1, -1), Material(RED)));
    }

    void LoadTestScene(){
        // brdf
        Sphere* s2 = new Sphere(vec3(0.0, -0.3, 0.0), 0.4, YELLOW);
        //        Sphere* s2 = new Sphere(vec3(0.0, -0.3, 0.0), 0.4, YELLOW, DIFF, 0, 1, vec3(0.15, -0.1, 0.1));

        s2->material.specularRate = 0.9;
        s2->material.roughness = 0.2;
        s2->material.metallic = 0.7;

        shapes.push_back(s2);

        // 发光物
        Triangle* l1 = new Triangle(vec3(0.4, 0.99, 0.4), vec3(-0.4, 0.99, -0.4), vec3(-0.4, 0.99, 0.4), Material(WHITE));
        Triangle* l2 = new Triangle(vec3(0.4, 0.99, 0.4), vec3(0.4, 0.99, -0.4), vec3(-0.4, 0.99, -0.4), Material(WHITE));
        l1->material.isEmissive = true;
        l2->material.isEmissive = true;
        lights.push_back(l1);
        lights.push_back(l2);
        shapes.push_back(l1);
        shapes.push_back(l2);

        // 背景盒子
        // bottom
        shapes.push_back(new Triangle(vec3(1, -1, 1), vec3(-1, -1, -1), vec3(-1, -1, 1), Material(WHITE)));
        shapes.push_back(new Triangle(vec3(1, -1, 1), vec3(1, -1, -1), vec3(-1, -1, -1), Material(WHITE)));
        // top
        shapes.push_back(new Triangle(vec3(1, 1, 1), vec3(-1, 1, 1), vec3(-1, 1, -1), Material(WHITE)));
        shapes.push_back(new Triangle(vec3(1, 1, 1), vec3(-1, 1, -1), vec3(1, 1, -1), Material(WHITE)));
        // back
        shapes.push_back(new Triangle(vec3(1, -1, -1), vec3(-1, 1, -1), vec3(-1, -1, -1), Material(CYAN)));
        shapes.push_back(new Triangle(vec3(1, -1, -1), vec3(1, 1, -1), vec3(-1, 1, -1), Material(CYAN)));
        // left
        shapes.push_back(new Triangle(vec3(-1, -1, -1), vec3(-1, 1, 1), vec3(-1, -1, 1), Material(BLUE)));
        shapes.push_back(new Triangle(vec3(-1, -1, -1), vec3(-1, 1, -1), vec3(-1, 1, 1), Material(BLUE)));
        // right
        shapes.push_back(new Triangle(vec3(1, 1, 1), vec3(1, -1, -1), vec3(1, -1, 1), Material(RED)));
        shapes.push_back(new Triangle(vec3(1, -1, -1), vec3(1, 1, 1), vec3(1, 1, -1), Material(RED)));
    }

    void testBeizer(){
        std::vector<vec3> controls = {vec3(-0.25, 0.25, 0), vec3(-0.5, 0, 0), vec3(-0.5, -0.25, 0), vec3(-0.25, -0.25, 0)};
        for(auto &c: controls){
            c += vec3(0, -0.75, 0);
        }
        auto* c = new BezierCurve(controls);
        auto m = Material(WHITE);
        m.specularRate = 0.9;
        m.roughness = 0.2;
        m.metallic = 0.8;
        auto f = new RevSurface(c, m);
        shapes.push_back(f);

        Sphere* s2 = new Sphere(vec3(0.0, -0.25, 0.0), 0.353, WHITE);
        s2->material.specularRate = 0.9;
        s2->material.roughness = 0.2;
        s2->material.metallic = 0.8;
        shapes.push_back(s2);

        // 发光物
        Triangle* l1 = new Triangle(vec3(0.4, 0.99, 0.4), vec3(-0.4, 0.99, -0.4), vec3(-0.4, 0.99, 0.4), Material(WHITE));
        Triangle* l2 = new Triangle(vec3(0.4, 0.99, 0.4), vec3(0.4, 0.99, -0.4), vec3(-0.4, 0.99, -0.4), Material(WHITE));
        l1->material.isEmissive = true;
        l2->material.isEmissive = true;
        lights.push_back(l1);
        lights.push_back(l2);
        shapes.push_back(l1);
        shapes.push_back(l2);

        // 背景盒子
        // bottom
        auto m2 = Material(WHITE);
        shapes.push_back(new Triangle(vec3(1, -1, 1), vec3(-1, -1, -1), vec3(-1, -1, 1), m2));
        shapes.push_back(new Triangle(vec3(1, -1, 1), vec3(1, -1, -1), vec3(-1, -1, -1), m2));
        // top
        shapes.push_back(new Triangle(vec3(1, 1, 1), vec3(-1, 1, 1), vec3(-1, 1, -1), Material(WHITE)));
        shapes.push_back(new Triangle(vec3(1, 1, 1), vec3(-1, 1, -1), vec3(1, 1, -1), Material(WHITE)));
        // back
        shapes.push_back(new Triangle(vec3(1, -1, -1), vec3(-1, 1, -1), vec3(-1, -1, -1), Material(WHITE)));
        shapes.push_back(new Triangle(vec3(1, -1, -1), vec3(1, 1, -1), vec3(-1, 1, -1), Material(WHITE)));
        // left
        shapes.push_back(new Triangle(vec3(-1, -1, -1), vec3(-1, 1, 1), vec3(-1, -1, 1), Material(RED)));
        shapes.push_back(new Triangle(vec3(-1, -1, -1), vec3(-1, 1, -1), vec3(-1, 1, 1), Material(RED)));
        // right
        shapes.push_back(new Triangle(vec3(1, 1, 1), vec3(1, -1, -1), vec3(1, -1, 1), Material(RED)));
        shapes.push_back(new Triangle(vec3(1, -1, -1), vec3(1, 1, 1), vec3(1, 1, -1), Material(RED)));
    }

    void testDepth(){

        // brdf
        Sphere* s2 = new Sphere(vec3(0.0, -0.3, 0.0), 0.1, YELLOW);
        //        Sphere* s2 = new Sphere(vec3(0.0, -0.3, 0.0), 0.4, YELLOW, DIFF, 0, 1, vec3(0.15, -0.1, 0.1));

        s2->material.specularRate = 0.9;
        s2->material.roughness = 0.2;
        s2->material.metallic = 0.7;

        shapes.push_back(s2);

        Sphere* s1 = new Sphere(vec3(0.2, -0.1, 0.2), 0.15, WHITE);
        shapes.push_back(s1);

        Sphere* s3 = new Sphere(vec3(-0.2, -0.5, -0.2), 0.05, WHITE);
        shapes.push_back(s3);

        // 发光物
        Triangle* l1 = new Triangle(vec3(0.4, 0.99, 0.4), vec3(-0.4, 0.99, -0.4), vec3(-0.4, 0.99, 0.4), Material(WHITE));
        Triangle* l2 = new Triangle(vec3(0.4, 0.99, 0.4), vec3(0.4, 0.99, -0.4), vec3(-0.4, 0.99, -0.4), Material(WHITE));
        l1->material.isEmissive = true;
        l2->material.isEmissive = true;
        lights.push_back(l1);
        lights.push_back(l2);
        shapes.push_back(l1);
        shapes.push_back(l2);

        // 背景盒子
        // bottom
        shapes.push_back(new Triangle(vec3(1, -1, 1), vec3(-1, -1, -1), vec3(-1, -1, 1), Material(WHITE)));
        shapes.push_back(new Triangle(vec3(1, -1, 1), vec3(1, -1, -1), vec3(-1, -1, -1), Material(WHITE)));
        // top
        shapes.push_back(new Triangle(vec3(1, 1, 1), vec3(-1, 1, 1), vec3(-1, 1, -1), Material(WHITE)));
        shapes.push_back(new Triangle(vec3(1, 1, 1), vec3(-1, 1, -1), vec3(1, 1, -1), Material(WHITE)));
        // back
        shapes.push_back(new Triangle(vec3(1, -1, -1), vec3(-1, 1, -1), vec3(-1, -1, -1), Material(CYAN)));
        shapes.push_back(new Triangle(vec3(1, -1, -1), vec3(1, 1, -1), vec3(-1, 1, -1), Material(CYAN)));
        // left
        shapes.push_back(new Triangle(vec3(-1, -1, -1), vec3(-1, 1, 1), vec3(-1, -1, 1), Material(BLUE)));
        shapes.push_back(new Triangle(vec3(-1, -1, -1), vec3(-1, 1, -1), vec3(-1, 1, 1), Material(BLUE)));
        // right
        shapes.push_back(new Triangle(vec3(1, 1, 1), vec3(1, -1, -1), vec3(1, -1, 1), Material(RED)));
        shapes.push_back(new Triangle(vec3(1, -1, -1), vec3(1, 1, 1), vec3(1, 1, -1), Material(RED)));
    }
};
