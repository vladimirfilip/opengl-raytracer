#ifndef OPENGL_RAYTRACER_CONSTANTS_H
#define OPENGL_RAYTRACER_CONSTANTS_H

#define TRIANGLE_SSBO_BINDING 1
#define TRIANGLE_NORMAL_SSBO_BINDING 2
#define BVH_BINDING 3

#define SCENE_FILE_PATH "../models/teapot.obj"

const float FOV = 90.0f;
const float VIEWPORT_DIST = 1.0f;
const unsigned int RAY_BOUNCES = 5;
const unsigned int RAYS_PER_PIXEL = 5;
const float CAMERA_MOVE_SPEED = 3.0f;
const int MAX_BVH_LEAF_TRIANGLE_COUNT = 4;
const int MAX_BVH_DEPTH = 32;
const int BVH_SPLIT_ITERATIONS = 100;

#endif //OPENGL_RAYTRACER_CONSTANTS_H
