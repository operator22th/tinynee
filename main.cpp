//独立实现
#include "include/renderer_legacy.h" //change this to include/renderer.h if you want to use the new renderer;
                                     // but remember to change light emission because the old one forget to
                                     // divide pdf
#include "include/scene.h"
#include "include/camera.h"

int main() {
    EasyScene scene;
    scene.testBeizer();

    vec3 eye = vec3(0, 0, 4.0);
    SimpleRenderer renderer;

//    ThinLensCamera camera(eye, vec3(0, 0, -1), 2.9);
    SimpleCamera camera(eye);
//    camera.focus(vec3(-1.0, -1, -1.0));
    renderer.render(scene, camera, 640, 640, 1, "beizer_final.png",false);
    return 0;
}