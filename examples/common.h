//
// Created by Бушев Дмитрий on 11.11.2021.
//

#ifndef RENDERAPITEST_COMMON_H
#define RENDERAPITEST_COMMON_H
#include <iostream>

#include "RenderInterface.h"
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "common/Model.h"

static void printHelp() {

  std::cout << "Vulkan VS OpenGL test suit." << std::endl
            << "  Usage: example [options]" << std::endl
            << "  Options:" << std::endl
            << "    -v     : Use vulkan backend" << std::endl
            << "    -ogl   : Use OpenGL backend" << std::endl
            << "    -val   : Enable Vulkan Validation Layer" << std::endl
            << "    --help : print help" << std::endl
            << "  Camera Controls: WASD + shift/ctrl for up/down move"
            << std::endl
            << "                   Toggle cursor(C) for camera rotation."
            << std::endl;
}

#define ROOT_SHADERS_DIR "shaders/"
class Camera {
  float fov = 60.0f;
  bool flipY_;
  float aspectRatio = 16.0f / 9.0f;
  float zNear = 0.1f, zFar = 100.0f;

  glm::vec3 position = glm::vec3{0.0f};
  glm::vec3 rotation = glm::vec3{0.0f};
  glm::mat4 perspective{1.0f};
  glm::mat4 cameraSpace{1.0f};
  APITest::UniformBufferRef cameraBuffer = nullptr;

  void setPerspective() {
    perspective = glm::perspective(glm::radians(fov), aspectRatio, zNear, zFar);
    if (flipY_)
      perspective[1][1] *= -1.0f;

    perspective =
        perspective * glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f),
                                  glm::vec3(1.0f, 0.0f, 0.0f));
  }

  void setCameraSpace() {
    glm::mat4 rotM = glm::mat4(1.0f);
    glm::mat4 transM;

    rotM = glm::rotate(rotM, glm::radians(-rotation.x),
                       glm::vec3(1.0f, 0.0f, 0.0f));
    rotM = glm::rotate(rotM, glm::radians(-rotation.y),
                       glm::vec3(0.0f, 1.0f, 0.0f));
    rotM = glm::rotate(rotM, glm::radians(-rotation.z),
                       glm::vec3(0.0f, 0.0f, 1.0f));

    glm::vec3 translation = -position;
    // translation.y = translation.y * -1.0f;

    transM = glm::translate(glm::mat4(1.0f), translation);

    cameraSpace = rotM * transM;
  }

  struct {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool space = false;
    bool shift = false;
  } keys;

  constexpr static const double rotSpeed = 0.1f;

  void rotate(double dx, double dy, double dz) {
    rotation.x += dx;
    rotation.y += dy;
    rotation.z += dz;

    if (rotation.x > 89.0f)
      rotation.x = 89.0f;
    if (rotation.x < -89.0f)
      rotation.x = -89.0f;
  }

public:
  explicit Camera(bool flipY) : flipY_(flipY) {
    setPerspective();
    setCameraSpace();
  }

  void allocateCameraBuffer(APITest::RenderInterfaceRef const &renderer) {
    cameraBuffer =
        renderer->createUniformBuffer(nullptr, sizeof(glm::mat4) * 2);
  }

  APITest::UniformBufferRef getCameraBuffer() const { return cameraBuffer; }
  glm::vec3 velocity = glm::vec3{0.0f};
  float inertia = 0.1f;
  float actingForce = 20.0f;
  float speed = 5.0f;

  glm::mat4 getCameraMatrix() const { return perspective * cameraSpace; }

  void updateFOV(float newFov) {
    fov = newFov;
    setPerspective();
  }

  void updateAspectRatio(float newAspectRatio) {
    aspectRatio = newAspectRatio;
    setPerspective();
  }

  void update(float deltaT) {

    float currentSpeed = glm::length(velocity);

    if (currentSpeed < 0.01f)
      velocity *= 0.0f;
    else
      velocity *= 1.0f - (1.5f) * deltaT / inertia;

    glm::vec3 camFront;
    camFront.x =
        glm::cos(glm::radians(rotation.x)) * glm::sin(glm::radians(rotation.z));
    camFront.z = -glm::sin(glm::radians(rotation.x));
    camFront.y = -glm::cos(glm::radians(rotation.x)) *
                 glm::cos(glm::radians(rotation.z));
    camFront = glm::normalize(-camFront);

    float acceleration = actingForce * deltaT / inertia;

    if (keys.up)
      velocity += camFront * acceleration;
    if (keys.down)
      velocity -= camFront * acceleration;
    if (keys.left)
      velocity -=
          glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 0.0f, 1.0f))) *
          acceleration;
    if (keys.right)
      velocity +=
          glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 0.0f, 1.0f))) *
          acceleration;
    if (keys.space)
      velocity -= glm::vec3(0.0f, 0.0f, 1.0f) * acceleration;
    if (keys.shift)
      velocity += glm::vec3(0.0f, 0.0f, 1.0f) * acceleration;

    position += velocity * (float)deltaT;

    setCameraSpace();

    if (cameraBuffer) {
      cameraBuffer->push(&cameraSpace, sizeof(cameraSpace), 0);
      cameraBuffer->push(&perspective, sizeof(perspective),
                         sizeof(cameraSpace));
    }
  };

  void keyDown(int key) {
    switch (key) {
    case APITest::KEY_W:
      keys.up = true;
      break;
    case APITest::KEY_A:
      keys.left = true;
      break;
    case APITest::KEY_S:
      keys.down = true;
      break;
    case APITest::KEY_D:
      keys.right = true;
      break;
    case APITest::KEY_LEFT_SHIFT:
      keys.shift = true;
      break;
    case APITest::KEY_SPACE:
      keys.space = true;
      break;
    default:;
    }
  }
  void keyUp(int key) {
    switch (key) {
    case APITest::KEY_W:
      keys.up = false;
      break;
    case APITest::KEY_A:
      keys.left = false;
      break;
    case APITest::KEY_S:
      keys.down = false;
      break;
    case APITest::KEY_D:
      keys.right = false;
      break;
    case APITest::KEY_LEFT_SHIFT:
      keys.shift = false;
      break;
    case APITest::KEY_SPACE:
      keys.space = false;
      break;
    default:;
    }
  }

  void mouseMove(double dx, double dy) {
    rotate(-dy * rotSpeed, 0.0f, dx * rotSpeed);
  }
};

class Model {};

#define MAIN_LOOP_HEAD()                                                       \
  while (!window->handleEvents() && !example_close) {                          \
    renderer->render();

#define MAIN_LOOP_TAIL()                                                       \
  }                                                                            \
  renderer->waitIdle();

#define EXAMPLE_MAIN_HEADER(EXAMPLE_NAME)                                      \
  int main(int argc, char **argv) {                                            \
                                                                               \
    bool use_vulkan = true, enable_validation = false;                         \
    bool example_close = false;                                                \
    for (auto i = 1; i < argc; ++i) {                                          \
      std::string curArg(argv[i]);                                             \
      if (curArg == "-v") {                                                    \
        use_vulkan = true;                                                     \
        continue;                                                              \
      }                                                                        \
      if (curArg == "-ogl") {                                                  \
        use_vulkan = false;                                                    \
        continue;                                                              \
      }                                                                        \
      if (curArg == "-val") {                                                  \
        enable_validation = true;                                              \
        continue;                                                              \
      }                                                                        \
      if (curArg == "--help") {                                                \
        printHelp();                                                           \
        return 0;                                                              \
      }                                                                        \
    }                                                                          \
    if (!use_vulkan && enable_validation)                                      \
      std::cout << "hint: -val option is Vulkan only." << std::endl;           \
    auto window = APITest::createGLFWWindow(                                   \
        {1024, 720,                                                            \
         use_vulkan ? "Vulkan " #EXAMPLE_NAME : "OpenGL " #EXAMPLE_NAME});     \
    auto renderer = use_vulkan                                                 \
                        ? APITest::createVulkanAPI(window, enable_validation)  \
                        : APITest::createOpenGLAPI(window);

#endif // RENDERAPITEST_COMMON_H
