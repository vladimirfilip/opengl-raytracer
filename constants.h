#ifndef OPENGL_RAYTRACER_CONSTANTS_H
#define OPENGL_RAYTRACER_CONSTANTS_H

#define TRIANGLE_SSBO_BINDING 1
#define TRIANGLE_NORMAL_SSBO_BINDING 2
#define BVH_BINDING 3
#define TRIANGLE_COLOUR_SSBO_BINDING 4

#define SCENE_FILE_PATH "../models/Dragon_80K.obj"

#define RENDER_MODE 1
#define TRIANGLE_TEST_MODE 2
#define BOX_TEST_MODE 3

const float FOV = 90.0f;
const float VIEWPORT_DIST = 0.1f;
const unsigned int RAY_BOUNCES = 5;
const unsigned int RAYS_PER_PIXEL = 5;
const float CAMERA_MOVE_SPEED = 1.0f;
const int MAX_BVH_LEAF_TRIANGLE_COUNT = 8;
const int MAX_BVH_DEPTH = 32;
const int BVH_SPLIT_ITERATIONS = 20;

const glm::vec3 CAMERA_START_POS(0.0f, 0.0f, -2.0f);

#endif //OPENGL_RAYTRACER_CONSTANTS_H
