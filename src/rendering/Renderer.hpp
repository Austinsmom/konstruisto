#ifndef RENDERING_RENDERER_HPP
#define RENDERING_RENDERER_HPP

#include <string>
#include <vector>

#include <GL/glew.h>
#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <nanovg.h>

#include "../engine/DebugInfo.hpp"
#include "../engine/Engine.hpp"
#include "../input/Selection.hpp"
#include "../input/WindowHandler.hpp"
#include "../world/World.hpp"
#include "ShaderManager.hpp"

namespace rendering {

class Renderer {

public:
  Renderer(engine::Engine& engine, world::World& world, input::Selection& selection);

  bool init();
  void cleanup();

  void renderWorld(bool renderNormals);

protected:
  engine::Engine& engine;
  world::World& world;
  input::Selection& selection;

  glm::vec3 clearColor;

  // UI
  // TODO(kantoniak): Move UI renderer to its class
  NVGcontext* nvgContext;
  static constexpr const char* FONT_SSP_REGULAR = "Source Sans Pro Regular";
  static constexpr const char* FONT_SSP_REGULAR_PATH = "assets/fonts/SourceSansPro/SourceSansPro-Regular.ttf";

  // Terrain
  GLuint shaderProgram;
  GLuint transformLoc, terrainPositionLoc, selectionLoc;
  GLuint VBO, VAO, terrainPositionVBO;
  GLuint texture;

  // Buildings
  GLuint buildingsVAO, buildingsVBO;
  GLuint buildingsInstanceVBO;
  GLuint buildingsShaderProgram;
  GLuint buildingsTransformLoc;
  const unsigned int buildingsCount = 100;
  const unsigned int sideSize = 20;
  const unsigned int maxHeight = 6;

  // Buildings - normals
  GLuint buildingNormalsTransformLoc;
  GLuint buildingNormalsShaderProgram;
};
}

#endif
