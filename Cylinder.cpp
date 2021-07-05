/*----------------------------------------------------------
* COSC363  Ray Tracer
*
*  The Cylinder class
*  This is a subclass of Object, and hence implements the
*  methods intersect() and normal().
-------------------------------------------------------------*/

#include "Cylinder.h"
#include <math.h>

/**
* Cylinder's intersection method.  The input is a ray.
*/
float Cylinder::intersect(glm::vec3 p0, glm::vec3 dir) {
    float t = -1.0;
    float cap_t = -1.0;
    float a = pow(dir.x, 2) + pow(dir.z, 2);
    float b = 2 * ((p0.x - center.x) * dir.x + (p0.z - center.z) * dir.z);
    float c = pow((p0.x - center.x), 2) + pow((p0.z - center.z), 2) - pow(radius, 2);
    float delta = b * b - (4 * a * c);

    if (delta < 0.001) return -1.0;    //includes zero and negative values
    float t1 = (-b - sqrt(delta)) / (2 * a);
    float t2 = (-b + sqrt(delta)) / (2 * a);

    glm::vec3 cap_center(center.x, center.y + height, center.z);
    glm::vec3 cap_n(0, 1, 0);
    glm::vec3 vdif = cap_center - p0;
    float vdotn = glm::dot(dir, cap_n);

    if (fabs(vdotn) > 0.001) {
        cap_t = glm::dot(vdif, cap_n) / vdotn;
        if (fabs(cap_t) >= 0.001) {
            glm::vec3 q3 = p0 + cap_t * dir;
            if (glm::length(cap_center - q3) > radius) {
                cap_t = -1;
            }
        }
    }

    glm::vec3 q1 = p0 + t1 * dir;
    glm::vec3 q2 = p0 + t2 * dir;

    if (q1.y > q2.y) {
        if (q1.y > center.y + height && q2.y > center.y + height) {
            t = -1.0;
        } else if (q1.y > center.y + height && (q2.y >= center.y && q2.y <= center.y + height)) {
            t = t2;
        } else if (q1.y >= center.y && q1.y <= center.y + height) {
            t = t1;
        }
    } else if (q1.y < q2.y) {
        if (q1.y > center.y + height && q2.y > center.y + height) {
            t = -1.0;
        } else {
            t = t1;
        }
    } else {
        t = t1 < t2 ? t1 : t2;
    }

    if (cap_t < 0 && t > 0) {
        return t;
    } else if (cap_t > 0 && t < 0) {
        return cap_t;
    } else if (cap_t > 0 && t > 0) {
        return t < cap_t ? t : cap_t;
    }
    return -1;
}

/**
* Returns the unit normal vector at a given point.
* Assumption: The input point p lies on the cylinder.
*/
glm::vec3 Cylinder::normal(glm::vec3 p) {
    if (p.y > center.y + height - 0.001) {
        return glm::vec3(0, 1, 0);
    }
    glm::vec3 n(((p.x - center.x) / radius), 0, ((p.z - center.z) / radius));
    return n;
}
