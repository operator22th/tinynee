#pragma once

#include "shape.h"
#include <vector>

#include <algorithm>
using namespace std;

// The CurvePoint object stores information about a point on a curve
// after it has been tesselated: the vertex (V) and the tangent (T)
// It is the responsiblility of functions that create these objects to fill in all the data.
struct CurvePoint {
    vec3 V; // Vertex
    vec3 T; // Tangent  (unit)
};

class Curve : public Shape {
public:
    std::vector<vec3> controls;
    float ymin, ymax, radius;

    explicit Curve(std::vector<vec3> points) : controls(std::move(points)) {
        ymin = INF;
        ymax = -INF;
        radius = 0;
        for (auto pt : controls) {
            ymin = min(pt.y, ymin);
            ymax = max(pt.y, ymax);
            radius = max(radius, fabs(pt.x));
            radius = max(radius, fabs(pt.z));
        }
    }

    HitResult intersect(Ray r) override {
        return HitResult();
    }

    std::vector<vec3> &getControls() {
        return controls;
    }

    virtual void discretize(int resolution, std::vector<CurvePoint>& data) = 0;
};

class BezierCurve : public Curve {
public:
    explicit BezierCurve(const std::vector<vec3> &points) : Curve(points) {
        if (points.size() < 4 || points.size() % 3 != 1) {
            printf("Number of control points of BezierCurve must be 3n+1!\n");
            exit(0);
        }
    }

    //3次曲线
    CurvePoint getPoint(float t){
        vec3 V = vec3 (0, 0, 0);
        vec3 T = vec3 (0, 0, 0);

        for (int k = 0; k <= 3; k++) {
            V += controls[k] * BezierBasis(k, t);
            T += controls[k] * BezierBasisDerivative(k, t);
        }
        T = normalize(T);
        return {V, T};
    }

    void discretize(int resolution, std::vector<CurvePoint>& data) override {
        data.clear();
        int n = controls.size() - 1;
        int group = n / 3;
        for(int i = 0; i < group; i++) {
            for(int j = (i == 0 ? 0: 1); j <= resolution; j++) {
                float t = (float)j / resolution;
                vec3 V = vec3 (0, 0, 0);
                vec3 T = vec3 (0, 0, 0);
                for(int k = 0; k <= 3; k++) {
                    V += controls[i * 3 + k] * BezierBasis(k, t);
                    T += controls[i * 3 + k] * BezierBasisDerivative(k, t);
                }
                T = normalize(T);
                data.push_back({V, T});
            }
        }
    }

protected:
    // n is fixed to 3 in this lab
    float BezierBasis(int i, float t) {
        if(i == 0)
            return (1 - t) * (1 - t) * (1 - t);
        if(i == 1)
            return 3 * t * (1 - t) * (1 - t);
        if(i == 2)
            return 3 * t * t * (1 - t);
        if(i == 3)
            return t * t * t;
        return 0;
    }

    float BezierBasisDerivative(int i, float t) {
        if(i == 0)
            return -3 * (1 - t) * (1 - t);
        if(i == 1)
            return 3 * (1 - 4 * t + 3 * t * t);
        if(i == 2)
            return 3 * (2 * t - 3 * t * t);
        if(i == 3)
            return 3 * t * t;
        return 0;
    }
};

class BsplineCurve : public Curve {
public:
    BsplineCurve(const std::vector<vec3> &points) : Curve(points) {
        if (points.size() < 4) {
            printf("Number of control points of BspineCurve must be more than 4!\n");
            exit(0);
        }
        n = controls.size() - 1;
    }

    void discretize(int resolution, std::vector<CurvePoint>& data) override {
        data.clear();
        for(int i = k; i <= n; i++) {
            for(int j = (i == 0 ? 0: 1); j <= resolution; j++) {
                float t = i + (float)j / resolution;
                t = t/(n + k + 1.0);
                CurvePoint cp = BsplineBasis(t);
                data.push_back(cp);
            }
        }
    }

protected:
    int n;
    int k = 3; // cubic B-spline in this lab

    CurvePoint BsplineBasis(float t) {
        vector<vector<float>> B(n + k + 1, vector<float>(k + 1, 0));
        //init
        float t0 = t * (n + k + 1.0);
        for(int i = 0; i <= n + k; i++) {
            if(i <= t0 && t0 < i + 1) {
                B[i][0] = 1;
            }
        }

        for(int p = 1; p <= k; p++) {
            for(int i = 0; i <= n + k - p; i++) {
                B[i][p] = B[i][p - 1] * (t0 - i) / (float)p
                          + B[i + 1][p - 1] * (i + p +1.0 - t0) / (float)p;
            }
        }
        vec3 V = vec3 (0, 0, 0);
        vec3 T = vec3 (0, 0, 0);
        for(int i = 0; i <= n; i++) {
            V += controls[i] * B[i][k];
            T += controls[i] * (n + k + 1.0f) * (B[i][k - 1] - B[i + 1][k - 1]);
        }
        return {V, T};
    }
};
