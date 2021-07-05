/*==================================================================================
* COSC 363  Computer Graphics (2021)
* Department of Computer Science and Software Engineering, University of Canterbury.
*
* A basic ray tracer
*===================================================================================
*/
#include <iostream>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include "Sphere.h"
#include "SceneObject.h"
#include "Ray.h"
#include <GL/freeglut.h>
#include "Plane.h"
#include "TextureBMP.h"
#include "Cone.h"
#include "Cylinder.h"

using namespace std;
const float WIDTH = 20.0;
const float HEIGHT = 20.0;
const float EDIST = 40.0;
const int NUMDIV = 500;
const int MAX_STEPS = 5;
const float XMIN = -WIDTH * 0.5;
const float XMAX = WIDTH * 0.5;
const float YMIN = -HEIGHT * 0.5;
const float YMAX = HEIGHT * 0.5;
const float ETA = 0.35;
const float fog_range[2] = {-30., -550.};
vector<SceneObject *> sceneObjects;
TextureBMP texture[2];

//---The most important function in a ray tracer! ----------------------------------
//   Computes the colour value obtained by tracing a ray and finding its
//     closest point of intersection with objects in the scene.
//----------------------------------------------------------------------------------
glm::vec3 trace(Ray ray, int step) {
    glm::vec3 backgroundCol(0);                     //Background colour = (0,0,0)
    glm::vec3 lightPos1(-5, 40, -5);                 //Light's position
    glm::vec3 lightPos2(30, 40, -5);
    glm::vec3 color(0);
    SceneObject *obj;
    ray.closestPt(sceneObjects);                    //Compare the ray with all objects in the scene
    if (ray.index == -1) return backgroundCol;       //no intersection
    obj = sceneObjects[ray.index];                  //object on which the closest point of intersection is found

    // A sphere textured using an image ---------------------------------
    if (ray.index == 1) {
        glm::vec3 n = obj->normal(ray.hit);
        float s = atan2(n.x, n.z) / (2 * M_PI) + 0.5;
        float t = -asin(n.y) / M_PI + 0.5;
        if (s >= 0 && s <= 1 && t >= 0 && t <= 1) {
            obj->setColor(texture[0].getColorAt(s, t));
        }
    }

    // floor -----------------------------------------------------------------
    if (ray.index == 4) {
        //Stripe pattern
        int stripeWidth = 5;
        int iz = (ray.hit.z) / stripeWidth;
        int ix = (ray.hit.x + 200) / stripeWidth;
        int k = (iz + ix) % 2; //2 colors
        if (k) obj->setColor(glm::vec3(0.6, 0.4, 0.9));
        else obj->setColor(glm::vec3(0.9, 0.6, 0.9));
    }
    // wall ------------------------------------------------------------------
    if (ray.index == 5) {
        float texcoords = (ray.hit.x - (-200)) / 400;
        float texcoordt = (ray.hit.y - (-15)) / 160;
        if (texcoords > 0 && texcoords < 1 && texcoordt > 0 && texcoordt < 1) {
            obj->setColor(texture[1].getColorAt(texcoords, texcoordt));
        }
    }

    // shadow ------------------------------------------------------------
    // Multiple light sources -----------------------------------------------
    glm::vec3 color_1 = obj->lighting(lightPos1, -ray.dir, ray.hit);
    glm::vec3 lightVec1 = lightPos1 - ray.hit;
    Ray shadowRay1(ray.hit, lightVec1);
    shadowRay1.closestPt(sceneObjects);
    float lightDist1 = glm::length(lightVec1);
    glm::vec3 color_2 = obj->lighting(lightPos2, -ray.dir, ray.hit);
    glm::vec3 lightVec2 = lightPos2 - ray.hit;
    Ray shadowRay2(ray.hit, lightVec2);
    shadowRay2.closestPt(sceneObjects);
    float lightDist2 = glm::length(lightVec2);

    // ambient lighting
    glm::vec3 surface_color(0);
    surface_color += 0.1f * obj->getColor();
    if (!(shadowRay1.index > -1 && shadowRay1.dist < lightDist1)) {
        // object is directly lit by this light
        surface_color += 0.45f * color_1;
    } else if (sceneObjects[shadowRay1.index]->isRefractive() || sceneObjects[shadowRay1.index]->isTransparent()) {
        surface_color += 0.25f * color_1;
    }
    if (!(shadowRay2.index > -1 && shadowRay2.dist < lightDist2)) {
        surface_color += 0.45f * color_2;
    } else if (sceneObjects[shadowRay2.index]->isRefractive() || sceneObjects[shadowRay2.index]->isTransparent()) {
        surface_color += 0.25f * color_2;
    }

    // Reflective ------------------------------------------------------
    if (obj->isReflective() && step < MAX_STEPS) {
        float rho = obj->getReflectionCoeff();
        glm::vec3 normalVec = obj->normal(ray.hit);
        glm::vec3 reflectedDir = glm::reflect(ray.dir, normalVec);
        Ray reflectedRay(ray.hit, reflectedDir);
        glm::vec3 reflectedColor = trace(reflectedRay, step + 1);
        color = (1 - rho) * surface_color + rho * reflectedColor;
    }

        // Refractive --------------------------------------------------------
    else if (obj->isRefractive() && step < MAX_STEPS) {
        float eta = obj->getRefractiveIndex() / 1.0f;
        glm::vec3 n = obj->normal(ray.hit);
        glm::vec3 g = glm::refract(ray.dir, n, eta);
        Ray refrRay(ray.hit, g);
        refrRay.closestPt(sceneObjects);
        glm::vec3 m = obj->normal(refrRay.hit);
        glm::vec3 h = glm::refract(g, -m, 1.0f / eta);
        Ray secondRefrRay(refrRay.hit, h);
        glm::vec3 refractedColor = trace(secondRefrRay, step + 1);
        float refr_coeff = obj->getRefractionCoeff();
        color = (1 - refr_coeff) * surface_color + refr_coeff * refractedColor;

    }

        // Transparent --------------------------------------------------------
    else if (obj->isTransparent() && step < MAX_STEPS) {
        Ray refrRay(ray.hit, ray.hit);
        refrRay.closestPt(sceneObjects);
        Ray secondRefrRay(refrRay.hit, ray.hit);
        glm::vec3 transparentColor = trace(secondRefrRay, step + 1);
        float tran_coeff = obj->getTransparencyCoeff();
        color = (1 - tran_coeff) * surface_color + tran_coeff * transparentColor;
    } else {
        // Regular surface (not transparent, reflective, or refractive).
        color = surface_color;
    }

    //fog -------------------------------------------------------------------------
    float fog_factor = (ray.hit.z - fog_range[0]) / (fog_range[1] - fog_range[0]);
    color = (1 - fog_factor) * color + fog_factor * glm::vec3(1., 1., 1.);
    return color;
}

//---The main display module -----------------------------------------------------------
// In a ray tracing application, it just displays the ray traced image by drawing
// each cell as a quad.
//---------------------------------------------------------------------------------------
void display() {
    float xp, yp;  //grid point
    float cellX = (XMAX - XMIN) / NUMDIV;  //cell width
    float cellY = (YMAX - YMIN) / NUMDIV;  //cell height
    glm::vec3 eye(0., 0., 0.);

    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glBegin(GL_QUADS);  //Each cell is a tiny quad.
    for (int i = 0; i < NUMDIV; i++) //Scan every cell of the image plane
    {
        xp = XMIN + i * cellX;
        for (int j = 0; j < NUMDIV; j++) {
            yp = YMIN + j * cellY;
            //Anti-Aliasing --------------------------------------------------------------
            glm::vec3 dir_1(xp + 0.25 * cellX, yp + 0.25 * cellY, -30);  //direction of the primary ray
            Ray ray_1 = Ray(eye, dir_1);
            glm::vec3 col_1 = trace(ray_1, 1); //Trace the primary ray and get the colour value

            glm::vec3 dir_2(xp + 0.75 * cellX, yp + 0.25 * cellY, -30);
            Ray ray_2 = Ray(eye, dir_2);
            glm::vec3 col_2 = trace(ray_2, 1);

            glm::vec3 dir_3(xp + 0.75 * cellX, yp + 0.75 * cellY, -30);
            Ray ray_3 = Ray(eye, dir_3);
            glm::vec3 col_3 = trace(ray_3, 1);

            glm::vec3 dir_4(xp + 0.25 * cellX, yp + 0.75 * cellY, -30);
            Ray ray_4 = Ray(eye, dir_4);
            glm::vec3 col_4 = trace(ray_4, 1);

            float col_r = (col_1.r + col_2.r + col_3.r + col_4.r) / 4;
            float col_g = (col_1.g + col_2.g + col_3.g + col_4.g) / 4;
            float col_b = (col_1.b + col_2.b + col_3.b + col_4.b) / 4;
            glColor3f(col_r, col_g, col_b);
            // -------------------------------------------------------------------------------
            glVertex2f(xp, yp);             //Draw each cell with its color value
            glVertex2f(xp + cellX, yp);
            glVertex2f(xp + cellX, yp + cellY);
            glVertex2f(xp, yp + cellY);
        }
    }
    glEnd();
    glFlush();
}

//---This function initializes the scene -------------------------------------------
//   Specifically, it creates scene objects (spheres, planes, cones, cylinders etc)
//     and add them to the list of scene objects.
//   It also initializes the OpenGL orthographc projection matrix for drawing the
//     the ray traced image.
//----------------------------------------------------------------------------------
void initialize() {
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(XMIN, XMAX, YMIN, YMAX);
    glClearColor(0, 0, 0, 1);

    Sphere *sphere1 = new Sphere(glm::vec3(-5.0, 0.0, -90.0), 13.0);
    sphere1->setReflectivity(true, 0.8);
    sphere1->setColor(glm::vec3(0.8, 0.7, 0.8));
    sceneObjects.push_back(sphere1);         //Add sphere1 to scene objects

    Sphere *sphere2 = new Sphere(glm::vec3(10.0, 13.0, -60.0), 3.0);
    sphere2->setReflectivity(false);
    sceneObjects.push_back(sphere2);         //Add sphere2 to scene objects

    Sphere *sphere3 = new Sphere(glm::vec3(5.0, 5.0, -70.0), 4.0);
    sphere3->setRefractivity(true, 1, 1.043169115);
    sphere3->setReflectivity(false);
    sceneObjects.push_back(sphere3);         //Add sphere3 to scene objects

    Sphere *sphere4 = new Sphere(glm::vec3(25., 3., -100.0), 5);
    sphere4->setTransparency(true, 0.6);
    sphere4->setReflectivity(false);
    sceneObjects.push_back(sphere4);         //Add sphere4 to scene objects

    // floor ----------------------------------------------------------
    Plane *plane = new Plane(glm::vec3(-200., -15, -20), //Point A
                             glm::vec3(200., -15, -20), //Point B
                             glm::vec3(200., -15, -420), //Point C
                             glm::vec3(-200., -15, -420)); //Point D

    plane->setColor(glm::vec3(0.8, 0.8, 0));
    sceneObjects.push_back(plane);
    plane->setSpecularity(false);

    // wall -----------------------------------------------------------
    Plane *wall = new Plane(glm::vec3(-200., -15, -420),
                            glm::vec3(200., -15, -420),
                            glm::vec3(200., 145, -420),
                            glm::vec3(-200., 145, -420));

    wall->setColor(glm::vec3(0.8, 0.8, 0));
    sceneObjects.push_back(wall);
    wall->setSpecularity(false);

    // box ------------------------------------------------------------
    Plane *box1 = new Plane(glm::vec3(-10., -12, -50), //Point a
                            glm::vec3(-5., -12, -50), //Point b
                            glm::vec3(-5., -12, -55), //Point c
                            glm::vec3(-10., -12, -55)); //Point d
    box1->setReflectivity(true, 0.8);
    box1->setColor(glm::vec3(0.8, 0.7, 0.8));
    sceneObjects.push_back(box1);

    Plane *box2 = new Plane(glm::vec3(-10., -7, -50), //Point e
                            glm::vec3(-5., -7, -50), //Point f
                            glm::vec3(-5., -7, -55), //Point g
                            glm::vec3(-10., -7, -55)); //Point h
    box2->setReflectivity(true, 0.8);
    box2->setColor(glm::vec3(0.8, 0.7, 0.8));
    sceneObjects.push_back(box2);

    Plane *box3 = new Plane(glm::vec3(-10., -7, -55), //Point h
                            glm::vec3(-10., -12, -55), //Point d
                            glm::vec3(-5., -12, -55), //Point c
                            glm::vec3(-5., -7, -55)); //Point g
    box3->setReflectivity(true, 0.8);
    box3->setColor(glm::vec3(0.8, 0.7, 0.8));
    sceneObjects.push_back(box3);

    Plane *box4 = new Plane(glm::vec3(-10., -7, -50), //Point e
                            glm::vec3(-10., -12, -50), //Point a
                            glm::vec3(-10., -12, -55), //Point d
                            glm::vec3(-10., -7, -55)); //Point h
    box4->setReflectivity(true, 0.8);
    box4->setColor(glm::vec3(0.8, 0.7, 0.8));
    sceneObjects.push_back(box4);

    Plane *box5 = new Plane(glm::vec3(-5., -7, -50), //Point f
                            glm::vec3(-5., -12, -50), //Point b
                            glm::vec3(-5., -12, -55), //Point c
                            glm::vec3(-5., -7, -55)); //Point g
    box5->setReflectivity(true, 0.8);
    box5->setColor(glm::vec3(0.8, 0.7, 0.8));
    sceneObjects.push_back(box5);

    Plane *box6 = new Plane(glm::vec3(-10., -7, -50), //Point e
                            glm::vec3(-10., -12, -50), //Point a
                            glm::vec3(-5., -12, -50), //Point b
                            glm::vec3(-5., -7, -50)); //Point f
    box6->setReflectivity(true, 0.8);
    box6->setColor(glm::vec3(0.8, 0.7, 0.8));
    sceneObjects.push_back(box6);

    Cone *cone1 = new Cone(glm::vec3(25., -12., -100.), 5, 10);
    cone1->setColor(glm::vec3(0.8, 0.7, 0.8));
    sceneObjects.push_back(cone1);         //Add cone to scene objects


    Cylinder *cylinder1 = new Cylinder(glm::vec3(7., -15., -60.), 2, 5);
    cylinder1->setReflectivity(true, 0.8);
    cylinder1->setColor(glm::vec3(0.8, 0.7, 0.8));
    sceneObjects.push_back(cylinder1);

    // TEXTURE --------------------------------------------------------
    texture[0] = TextureBMP("earth.bmp");
    texture[1] = TextureBMP("sky.bmp");
}

int main(int argc, char *argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(500, 500);
    glutInitWindowPosition(20, 20);
    glutCreateWindow("Raytracing");
    glutDisplayFunc(display);
    initialize();
    glutMainLoop();
    return 0;
}
