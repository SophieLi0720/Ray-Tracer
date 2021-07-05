/*----------------------------------------------------------
* COSC363  Ray Tracer
*
*  The Cone class
*  This is a subclass of Object, and hence implements the
*  methods intersect() and normal().
-------------------------------------------------------------*/

#include "Cone.h"
#include <math.h>

/**
* Cone's intersection method.  The input is a ray.
*/
float Cone::intersect(glm::vec3 p0, glm::vec3 dir) {
    float a = pow(dir.x, 2) + pow(dir.z, 2) - pow(radius / height, 2) * pow(dir.y, 2);
    float b = 2 * ((p0.x - center.x) * dir.x + (p0.z - center.z) * dir.z -
                   pow(radius / height, 2) * (-height + p0.y - center.y) * dir.y);
    float c = -pow(radius / height, 2) * pow((height - p0.y + center.y), 2) + pow((p0.x - center.x), 2) +
              pow((p0.z - center.z), 2);
    float delta = b * b - (4 * a * c);

    if (delta < 0.001) return -1.0;    //includes zero and negative values
    float t1 = (-b - sqrt(delta)) / (2 * a);
    float t2 = (-b + sqrt(delta)) / (2 * a);
    glm::vec3 q = p0 + t1 * dir;
    if (q.y >= center.y && q.y <= center.y + height) {
        return t1;
    }

    q = p0 + t2 * dir;
    if (q.y >= center.y && q.y <= center.y + height) {
        return t2;
    }
    return -1;
}

/**
* Returns the unit normal vector at a given point.
* Assumption: The input point p lies on the Cone.
*/
glm::vec3 Cone::normal(glm::vec3 p) {
    float alpha = atan2(p.x - center.x, p.z - center.z);
    float theta = atan2(radius, height);
    glm::vec3 n(sin(alpha) * cos(theta), sin(theta), cos(alpha) * cos(theta));
    return n;
}
