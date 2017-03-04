#ifndef STATES_TESTSTATE_HPP
#define STATES_TESTSTATE_HPP

#include <chrono>
#include <cmath>
#include <iostream>

#include "../data/CameraState.hpp"
#include "../engine/Engine.hpp"
#include "../engine/GameState.hpp"
#include "../input/Selection.hpp"
#include "../input/WindowHandler.hpp"
#include "../rendering/Renderer.hpp"
#include "../world/Camera.hpp"
#include "../world/Geometry.hpp"
#include "../world/World.hpp"

namespace states {

class TestState : public engine::GameState {

public:
  TestState(engine::Engine& engine);

  void init();
  void cleanup();

  void update(std::chrono::milliseconds delta);
  void render();

  void onKey(int key, int scancode, int action, int mods);
  void onMouseButton(int button, int action, int mods);
  void onScroll(double xoffset, double yoffset);
  void onWindowResize(int width, int height);

private:
  rendering::Renderer renderer;
  world::Geometry geometry;
  world::World world;
  input::Selection selection;

  void createRandomWorld();

  // TODO(kantoniak): Move actions to InputHandler
  glm::vec2 dragStart = glm::vec2();
  bool rmbPressed = false;
  bool rotatingClockwise = false;
  bool rotatingCounterClockwise = false;
  bool rotatingUpwards = false;
  bool rotatingDownwards = false;

  void handleMapDragging(std::chrono::milliseconds delta);
  void handleRotatingAroundY(std::chrono::milliseconds delta, bool clockwise);
  void handleRotatingAroundX(std::chrono::milliseconds delta, bool upwards);

  bool renderNormals = false;
};
}

#endif
