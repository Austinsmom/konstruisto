#include "WorldRenderer.hpp"

#include "../data/Chunk.hpp"

namespace rendering {
WorldRenderer::WorldRenderer(engine::Engine& engine, world::World& world) : Renderer(engine), world(world) {
  clearColor = glm::vec3(89, 159, 209) / 255.f;
}

bool WorldRenderer::init() {
  Renderer::init();

  if (!setupShaders() || !setupTextures()) {
    return false;
  }

  if (!setupTerrain() || !setupBuildings()) {
    return false;
  }

  glClearColor(clearColor.x, clearColor.y, clearColor.z, 1.f);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);

  return true;
}

bool WorldRenderer::setupShaders() {
  // TODO(kantoniak): Handle loader/compiler/linker failure when initializing shaders

  GLuint vertexShader = compileShader(GL_VERTEX_SHADER, "assets/shaders/terrain.vs");
  GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, "assets/shaders/terrain.fs");
  this->shaderProgram = ShaderManager::linkProgram(vertexShader, 0, fragmentShader, engine.getLogger());
  transformLoc = glGetUniformLocation(shaderProgram, "transform");
  renderGridLoc = glGetUniformLocation(shaderProgram, "renderGrid");
  selectionLoc = glGetUniformLocation(shaderProgram, "selection");
  selectionColorLoc = glGetUniformLocation(shaderProgram, "selectionColor");
  groundTextureLoc = glGetUniformLocation(shaderProgram, "groundTexture");
  roadTextureLoc = glGetUniformLocation(shaderProgram, "roadTexture");

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  GLuint buildingsVertexShader = compileShader(GL_VERTEX_SHADER, "assets/shaders/buildings.vs");
  GLuint buildingsGeomShader = compileShader(GL_GEOMETRY_SHADER, "assets/shaders/buildings.gs");
  GLuint buildingsFragmentShader = compileShader(GL_FRAGMENT_SHADER, "assets/shaders/buildings.fs");
  this->buildingsShaderProgram = ShaderManager::linkProgram(buildingsVertexShader, buildingsGeomShader,
                                                            buildingsFragmentShader, engine.getLogger());
  buildingsTransformLoc = glGetUniformLocation(buildingsShaderProgram, "transform");

  GLuint buildingNormalsGeomShader = compileShader(GL_GEOMETRY_SHADER, "assets/shaders/buildings_normals.gs");
  GLuint buildingsNormalFragmentShader = compileShader(GL_FRAGMENT_SHADER, "assets/shaders/buildings_normals.fs");
  this->buildingNormalsShaderProgram = ShaderManager::linkProgram(buildingsVertexShader, buildingNormalsGeomShader,
                                                                  buildingsNormalFragmentShader, engine.getLogger());
  buildingNormalsTransformLoc = glGetUniformLocation(buildingNormalsShaderProgram, "transform");

  glDeleteShader(buildingsVertexShader);
  glDeleteShader(buildingsGeomShader);
  glDeleteShader(buildingsVertexShader);
  glDeleteShader(buildingNormalsGeomShader);
  glDeleteShader(buildingsNormalFragmentShader);

  return true;
}

bool WorldRenderer::setupTextures() {
  {
    int width, height;
    unsigned char* pixels = stbi_load("assets/textures/grid.png", &width, &height, nullptr, STBI_rgb_alpha);

    // Texture of the ground
    glGenTextures(1, &gridTexture);
    glBindTexture(GL_TEXTURE_2D, gridTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(pixels);
  }

  {
    int width, height;
    unsigned char* pixels = stbi_load("assets/textures/road.png", &width, &height, nullptr, STBI_rgb_alpha);

    // Texture of the ground
    // TODO(kantoniak): Better system for atlas sizing
    glGenTextures(1, &roadTexture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, roadTexture);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, 320, 320, 1);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, 320, 320, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(pixels);
  }

  return true;
}

bool WorldRenderer::setupTerrain() {
  glGenVertexArrays(1, &VAO);
  markTileDataForUpdate();
  return true;
}

bool WorldRenderer::setupBuildings() {
  GLfloat building[] = {// 8x2
                        0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
                        1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
                        0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, // Reset
                        1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f};

  glGenVertexArrays(1, &buildingsVAO);
  glBindVertexArray(buildingsVAO);

  glGenBuffers(1, &buildingsVBO);
  glBindBuffer(GL_ARRAY_BUFFER, buildingsVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(building), building, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);

  glGenBuffers(1, &buildingsInstanceVBO);
  glBindBuffer(GL_ARRAY_BUFFER, buildingsInstanceVBO);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
  glVertexAttribDivisor(1, 1);

  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
  glVertexAttribDivisor(2, 1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  markBuildingDataForUpdate();

  return true;
}

void WorldRenderer::cleanup() {
  glUseProgram(0);

  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &terrainPositionVBO);
  glDeleteProgram(this->shaderProgram);

  for (auto it = chunks.begin(); it != chunks.end(); it++) {
    glDeleteBuffers(1, &(it->second));
  }

  glDeleteVertexArrays(1, &buildingsVAO);
  glDeleteBuffers(1, &buildingsVBO);
  glDeleteBuffers(1, &buildingsInstanceVBO);
  glDeleteProgram(this->buildingsShaderProgram);

  glDeleteProgram(this->buildingNormalsShaderProgram);

  Renderer::cleanup();
}

void WorldRenderer::markBuildingDataForUpdate() {
  resendBuildingData = true;
}

void WorldRenderer::markTileDataForUpdate() {
  resendTileData = true;
}

void WorldRenderer::renderWorld(const input::Selection& selection) {

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);

  const glm::mat4 vp = world.getCamera().getViewProjectionMatrix();

  // Terrain
  if (resendTileData) {
    sendTileData();
    resendTileData = false;
  }

  glUseProgram(shaderProgram);
  glBindVertexArray(VAO);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, gridTexture);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D_ARRAY, roadTexture);
  glUniform1i(groundTextureLoc, 0);
  glUniform1i(roadTextureLoc, 1);

  glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(vp));
  glUniform1i(renderGridLoc, engine.getSettings().world.showGrid);
  glUniform4i(selectionLoc, selection.getFrom().x, selection.getFrom().y, selection.getTo().x + 1,
              selection.getTo().y + 1);
  glUniform4f(selectionColorLoc, selection.getColor().x, selection.getColor().y, selection.getColor().z,
              engine.getSettings().rendering.renderSelection ? selection.getColor().w : 0);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  for (data::Chunk* chunk : world.getMap().getChunks()) {
    glBindBuffer(GL_ARRAY_BUFFER, chunks[std::make_pair(chunk->getPosition().x, chunk->getPosition().y)]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));

    glDrawArrays(GL_TRIANGLES, 0, data::Chunk::SIDE_LENGTH * data::Chunk::SIDE_LENGTH * 2 * 3);
  }
  glDisableVertexAttribArray(1);
  glDisableVertexAttribArray(0);

  // Buildings
  if (resendBuildingData) {
    sendBuildingData();
    resendBuildingData = false;
  }

  if (world.getMap().getBuildingCount() > 0) {
    glUseProgram(buildingsShaderProgram);
    glBindVertexArray(buildingsVAO);

    glUniformMatrix4fv(buildingsTransformLoc, 1, GL_FALSE, glm::value_ptr(vp));
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 16, world.getMap().getBuildingCount());
  }

  glBindVertexArray(0);
  glFlush();
}

void WorldRenderer::renderDebug() {
  if (engine.getSettings().rendering.renderNormals) {
    const glm::mat4 vp = world.getCamera().getViewProjectionMatrix();

    glUseProgram(buildingNormalsShaderProgram);
    glBindVertexArray(buildingsVAO);

    glUniformMatrix4fv(buildingsTransformLoc, 1, GL_FALSE, glm::value_ptr(vp));
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 16, world.getMap().getBuildingCount());

    glBindVertexArray(0);
    glFlush();
  }
}

void WorldRenderer::renderUI() {
  const glm::vec2 viewport = engine.getWindowHandler().getViewportSize();
  NVGcontext* context = engine.getUI().getContext();

  constexpr unsigned char topbarHeight = 36;
  constexpr unsigned char topbarInnerMargin = 12;
  constexpr unsigned char topbarOuterMargin = 6;

  const NVGcolor iconBackgroundColor = nvgRGB(68, 68, 68);

  const data::City& city = world.getMap().getCurrentCity();
  const std::string date = world.getTimer().getDate();
  const std::string people = "People: " + std::to_string(city.people);
  const std::string money = "€" + std::to_string(city.money);

  nvgTextAlign(context, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
  nvgFontFace(context, rendering::UI::FONT_SSP_BOLD);
  nvgFontSize(context, 22.0f);
  float cityNameWidth = nvgTextBounds(context, 0, 0, city.name.c_str(), nullptr, nullptr);

  nvgFontFace(context, rendering::UI::FONT_SSP_REGULAR);
  nvgFontSize(context, 19.0f);
  float dateWidth = nvgTextBounds(context, 0, 0, date.c_str(), nullptr, nullptr);
  float peopleWidth = nvgTextBounds(context, 0, 0, people.c_str(), nullptr, nullptr);
  float moneyWidth = nvgTextBounds(context, 0, 0, money.c_str(), nullptr, nullptr);

  const unsigned short cityNameBlockWidth = cityNameWidth + 2 * topbarInnerMargin;
  const unsigned short dateBlockWidth =
      (1 + world.getTimer().getMaxSpeed()) * (UI::ICON_SIDE + topbarInnerMargin) + dateWidth + 2 * topbarInnerMargin;
  const unsigned short cityNumbersBlockWidth = peopleWidth + moneyWidth + 3 * topbarInnerMargin;
  const unsigned short topbarWidth =
      cityNameBlockWidth + dateBlockWidth + cityNumbersBlockWidth + 2 * topbarOuterMargin;

  nvgBeginPath(context);
  nvgRect(context, viewport.x / 2 - topbarWidth / 2, 0, cityNameBlockWidth, topbarHeight);
  nvgRect(context, viewport.x / 2 - topbarWidth / 2 + cityNameBlockWidth + topbarOuterMargin, 0, dateBlockWidth,
          topbarHeight);
  nvgRect(context, viewport.x / 2 - topbarWidth / 2 + cityNameBlockWidth + dateBlockWidth + 2 * topbarOuterMargin, 0,
          cityNumbersBlockWidth, topbarHeight);
  nvgFillColor(context, engine.getUI().getBackgroundColor());
  nvgFill(context);

  // Icon backgrounds
  short speedIcon = world.getTimer().paused() ? 0 : world.getTimer().getSpeed();
  nvgBeginPath(context);
  nvgRect(context, viewport.x / 2 - topbarWidth / 2 + cityNameBlockWidth + topbarOuterMargin + dateWidth +
                       2 * topbarInnerMargin + speedIcon * (UI::ICON_SIDE + topbarInnerMargin),
          topbarHeight / 2 - UI::ICON_SIDE / 2, UI::ICON_SIDE, UI::ICON_SIDE);
  nvgFillColor(context, iconBackgroundColor);
  nvgFill(context);

  nvgFillColor(context, engine.getUI().getPrimaryTextColor());

  nvgFontFace(context, rendering::UI::FONT_SSP_BOLD);
  nvgFontSize(context, 22.0f);
  nvgText(context, viewport.x / 2 - topbarWidth / 2 + topbarInnerMargin, topbarHeight / 2, city.name.c_str(), nullptr);

  nvgFontFace(context, rendering::UI::FONT_SSP_REGULAR);
  nvgFontSize(context, 19.0f);
  nvgText(context, viewport.x / 2 - topbarWidth / 2 + cityNameBlockWidth + topbarOuterMargin + topbarInnerMargin,
          topbarHeight / 2, date.c_str(), nullptr);
  nvgText(context, viewport.x / 2 - topbarWidth / 2 + cityNameBlockWidth + dateBlockWidth + 2 * topbarOuterMargin +
                       topbarInnerMargin,
          topbarHeight / 2, people.c_str(), nullptr);
  nvgText(context, viewport.x / 2 - topbarWidth / 2 + cityNameBlockWidth + dateBlockWidth + 2 * topbarOuterMargin +
                       topbarInnerMargin * 2 + peopleWidth,
          topbarHeight / 2, money.c_str(), nullptr);

  // Speed numbers
  {
    int x =
        viewport.x / 2 - topbarWidth / 2 + cityNameBlockWidth + topbarOuterMargin + 2 * topbarInnerMargin + dateWidth;
    const int y = topbarHeight / 2 - UI::ICON_SIDE / 2;

    engine.getUI().renderIcon(UI::ICON_SPEED_0, x, y);
    x += UI::ICON_SIDE + topbarInnerMargin;
    engine.getUI().renderIcon(UI::ICON_SPEED_1, x, y);
    x += UI::ICON_SIDE + topbarInnerMargin;
    engine.getUI().renderIcon(UI::ICON_SPEED_2, x, y);
    x += UI::ICON_SIDE + topbarInnerMargin;
    engine.getUI().renderIcon(UI::ICON_SPEED_3, x, y);
  }

  // Left menu
  {
    const unsigned short optionsCount = 5;
    const unsigned short buttonSide = 3 * UI::ICON_SIDE;
    int x = 0;
    int y = viewport.y / 2 - optionsCount * buttonSide / 2.f;

    nvgBeginPath(context);
    nvgRect(context, x, y, buttonSide, buttonSide * optionsCount);
    nvgFillColor(context, engine.getUI().getBackgroundColor());
    nvgFill(context);

    if (leftMenuActiveIcon >= 0) {
      nvgBeginPath(context);
      nvgRect(context, x, y + buttonSide * leftMenuActiveIcon, buttonSide, buttonSide);
      nvgFillColor(context, iconBackgroundColor);
      nvgFill(context);
    }

    x = (buttonSide - UI::ICON_SIDE_24) / 2;
    y += x;

    engine.getUI().renderIcon(UI::ICON_BUILDING, x, y, UI::ICON_SIDE_24);
    y += buttonSide;
    engine.getUI().renderIcon(UI::ICON_ZONES, x, y, UI::ICON_SIDE_24);
    y += buttonSide;
    engine.getUI().renderIcon(UI::ICON_ROAD, x, y, UI::ICON_SIDE_24);
    y += buttonSide;
    engine.getUI().renderIcon(UI::ICON_BULDOZER, x, y, UI::ICON_SIDE_24);
    y += buttonSide;
    engine.getUI().renderIcon(UI::ICON_MORE, x, y, UI::ICON_SIDE_24);
  }
}

void WorldRenderer::renderDebugUI() {
  NVGcontext* context = engine.getUI().getContext();

  constexpr unsigned short margin = 10;
  constexpr unsigned short textMargin = 4;
  constexpr unsigned short lineHeight = 18;

  const std::string fps = "FPS " + std::to_string(engine.getDebugInfo().getFPS());
  const std::string frame = "Frame: " + std::to_string(engine.getDebugInfo().getFrameTime()) + " ms";
  const std::string render = "Render: " + std::to_string(engine.getDebugInfo().getRenderTime()) + " ms";
  const std::string renderWorld = "  World: " + std::to_string(engine.getDebugInfo().getRenderWorldTime()) + " ms";
  const std::string renderUI = "  UI: " + std::to_string(engine.getDebugInfo().getRenderUITime()) + " ms";

  nvgBeginPath(context);
  nvgRect(context, margin, margin, 100, 2 * textMargin + 5 * lineHeight);
  nvgFillColor(context, engine.getUI().getBackgroundColor());
  nvgFill(context);

  nvgFontSize(context, 16.0f);
  nvgFontFace(context, rendering::UI::FONT_SSP_REGULAR);
  nvgTextAlign(context, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
  nvgFillColor(context, engine.getUI().getPrimaryTextColor());
  nvgText(context, 1.8f * margin, margin + textMargin + lineHeight / 2.f, fps.c_str(), nullptr);
  nvgText(context, 1.8f * margin, margin + textMargin + lineHeight / 2.f + lineHeight, frame.c_str(), nullptr);
  nvgText(context, 1.8f * margin, margin + textMargin + lineHeight / 2.f + 2 * lineHeight, render.c_str(), nullptr);
  nvgText(context, 1.8f * margin, margin + textMargin + lineHeight / 2.f + 3 * lineHeight, renderWorld.c_str(),
          nullptr);
  nvgText(context, 1.8f * margin, margin + textMargin + lineHeight / 2.f + 4 * lineHeight, renderUI.c_str(), nullptr);
}

void WorldRenderer::setLeftMenuActiveIcon(int index) {
  leftMenuActiveIcon = index;
}

void WorldRenderer::sendBuildingData() {
  const unsigned int buildingCount = world.getMap().getBuildingCount();
  if (buildingCount < 1) {
    return;
  }

  std::vector<glm::vec3> buildingPositions;
  buildingPositions.reserve(2 * buildingCount);

  constexpr float buildingMargin = 0.2f;
  for (data::Chunk* chunk : world.getMap().getChunks()) {
    for (data::buildings::Building building : chunk->getResidentials()) {
      buildingPositions.push_back(glm::vec3(building.x + buildingMargin, 0, building.y + buildingMargin));
      buildingPositions.push_back(
          glm::vec3(building.width - 2 * buildingMargin, building.level, building.length - 2 * buildingMargin));
    }
  }
  glBindVertexArray(buildingsVAO);
  glBindBuffer(GL_ARRAY_BUFFER, buildingsInstanceVBO);
  glBufferDataVector(GL_ARRAY_BUFFER, buildingPositions, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void WorldRenderer::sendTileData() {
  glBindVertexArray(VAO);

  constexpr unsigned long verticesCount = data::Chunk::SIDE_LENGTH * data::Chunk::SIDE_LENGTH * 2 * 3;
  const std::vector<glm::vec3> fieldBase{glm::vec3(1, 0, 0), glm::vec3(0, 0, 0), glm::vec3(1, 0, 1),
                                         glm::vec3(1, 0, 1), glm::vec3(0, 0, 0), glm::vec3(0, 0, 1)};

  // Generate buffers
  std::vector<glm::vec3> positions;
  positions.resize(verticesCount);

  std::vector<GLfloat> tiles;
  tiles.resize(verticesCount);

  GLuint chunkVBO = 0;
  std::vector<GLfloat> toBuffer;
  toBuffer.resize(verticesCount * (3 + 1));

  for (data::Chunk* chunk : world.getMap().getChunks()) {
    glm::vec3 chunkPosition =
        glm::vec3(chunk->getPosition().x, 0, chunk->getPosition().y) * (float)data::Chunk::SIDE_LENGTH;

    // Generate positions
    for (unsigned int x = 0; x < data::Chunk::SIDE_LENGTH; x++) {
      for (unsigned int y = 0; y < data::Chunk::SIDE_LENGTH; y++) {
        for (unsigned int i = 0; i < 6; i++) {
          positions[y * data::Chunk::SIDE_LENGTH * 6 + x * 6 + i] = chunkPosition + glm::vec3(x, 0, y) + fieldBase[i];
        }
      }
    }

    // Generate tiles
    std::fill(tiles.begin(), tiles.end(), 0);

    glm::ivec2 toBottomRight = chunk->getPosition() - glm::ivec2(1, 1);
    if (world.getMap().chunkExists(toBottomRight)) {
      this->paintOnTiles(world.getMap().getChunk(toBottomRight), chunk->getPosition(), tiles);
    }

    glm::ivec2 toRight = chunk->getPosition() - glm::ivec2(1, 0);
    if (world.getMap().chunkExists(toRight)) {
      this->paintOnTiles(world.getMap().getChunk(toRight), chunk->getPosition(), tiles);
    }

    glm::ivec2 toBottom = chunk->getPosition() - glm::ivec2(0, 1);
    if (world.getMap().chunkExists(toBottom)) {
      this->paintOnTiles(world.getMap().getChunk(toBottom), chunk->getPosition(), tiles);
    }

    this->paintOnTiles(*chunk, chunk->getPosition(), tiles);

    // Concatenate
    for (unsigned int i = 0; i < verticesCount; i++) {
      toBuffer[i * 4] = positions[i].x;
      toBuffer[i * 4 + 1] = positions[i].y;
      toBuffer[i * 4 + 2] = positions[i].z;
      toBuffer[i * 4 + 3] = tiles[i];
    }

    // Send buffer
    auto key = std::make_pair(chunk->getPosition().x, chunk->getPosition().y);
    if (chunks.find(key) == chunks.end()) {
      glGenBuffers(1, &chunkVBO);
      chunks[key] = chunkVBO;
    } else {
      chunkVBO = chunks[key];
    }
    glBindBuffer(GL_ARRAY_BUFFER, chunkVBO);
    glBufferDataVector(GL_ARRAY_BUFFER, toBuffer, GL_STATIC_DRAW);
  }

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

unsigned int WorldRenderer::getTile(int x, int y) const {
  return y * ATLAS_SIDE + x + 1;
}

void WorldRenderer::setTile(std::vector<GLfloat>& tiles, int x, int y, unsigned int tile) {
  if (x < 0 || (int)data::Chunk::SIDE_LENGTH <= x || y < 0 || (int)data::Chunk::SIDE_LENGTH <= y) {
    return;
  }
  for (int i = 0; i < 6; i++) {
    tiles[(y * data::Chunk::SIDE_LENGTH + x) * 6 + i] = tile;
  }
}

void WorldRenderer::paintOnTiles(const data::Chunk& chunk, const glm::ivec2& position, std::vector<GLfloat>& tiles) {
  for (data::Lot lot : chunk.getLots()) {
    this->paintLotOnTiles(lot, position, tiles);
  }
  for (data::Road road : chunk.getRoads()) {
    this->paintRoadOnTiles(road, position, tiles);
  }
  for (data::RoadGraph::Node node : chunk.getRoadGraph().getNodes()) {
    this->paintRoadNodeOnTiles(node, position, tiles);
  }
}

void WorldRenderer::paintLotOnTiles(const data::Lot& lot, const glm::ivec2& position, std::vector<GLfloat>& tiles) {

  int minX = lot.position.getLocal(position).x;
  int minY = lot.position.getLocal(position).y;
  int maxX = minX + lot.size.x - 1;
  int maxY = minY + lot.size.y - 1;
  for (int x = minX + 1; x < maxX; x++) {
    setTile(tiles, x, minY, getTile(6, 2));
    for (int y = minY + 1; y < maxY; y++) {
      setTile(tiles, x, y, getTile(6, 3));
    }
    setTile(tiles, x, maxY, getTile(6, 4));
  }
  for (int y = minY + 1; y < maxY; y++) {
    setTile(tiles, minX, y, getTile(5, 3));
    setTile(tiles, maxX, y, getTile(7, 3));
  }
  setTile(tiles, minX, minY, getTile(5, 2));
  setTile(tiles, maxX, minY, getTile(7, 2));
  setTile(tiles, minX, maxY, getTile(5, 4));
  setTile(tiles, maxX, maxY, getTile(7, 4));

  // Render markers
  if (data::Direction::N == lot.direction) {
    setTile(tiles, minX, maxY, getTile(5, 1));
    for (int x = minX + 1; x < maxX; x++) {
      setTile(tiles, x, maxY, getTile(6, 1));
    }
    setTile(tiles, maxX, maxY, getTile(7, 1));
  }

  if (data::Direction::S == lot.direction) {
    setTile(tiles, minX, minY, getTile(5, 0));
    for (int x = minX + 1; x < maxX; x++) {
      setTile(tiles, x, minY, getTile(6, 0));
    }
    setTile(tiles, maxX, minY, getTile(7, 0));
  }

  if (data::Direction::W == lot.direction) {
    setTile(tiles, maxX, minY, getTile(9, 0));
    for (int y = minY + 1; y < maxY; y++) {
      setTile(tiles, maxX, y, getTile(9, 1));
    }
    setTile(tiles, maxX, maxY, getTile(9, 2));
  }

  if (data::Direction::E == lot.direction) {
    setTile(tiles, minX, minY, getTile(8, 0));
    for (int y = minY + 1; y < maxY; y++) {
      setTile(tiles, minX, y, getTile(8, 1));
    }
    setTile(tiles, minX, maxY, getTile(8, 2));
  }
}

void WorldRenderer::paintRoadOnTiles(data::Road& road, const glm::ivec2& position, std::vector<GLfloat>& tiles) {

  if (road.direction == data::Direction::N) {
    const int minX = road.position.getLocal(position).x;
    const int minY = road.position.getLocal(position).y;
    const int maxX = minX + road.getType().width - 1;
    const int maxY = minY + road.length - 1;

    for (int y = minY + 1; y < maxY; y++) {
      setTile(tiles, minX, y, getTile(0, 1));
      for (int x = minX + 1; x < maxX; x++) {
        setTile(tiles, x, y, getTile(1, 1));
      }
      setTile(tiles, maxX, y, getTile(2, 1));
    }
  }

  if (road.direction == data::Direction::W) {
    const int minX = road.position.getLocal(position).x;
    const int minY = road.position.getLocal(position).y;
    const int maxX = minX + road.length - 1;
    const int maxY = minY + road.getType().width - 1;

    for (int x = minX + 1; x < maxX; x++) {
      setTile(tiles, x, minY, getTile(1, 0));
      for (int y = minY + 1; y < maxY; y++) {
        setTile(tiles, x, y, getTile(1, 1));
      }
      setTile(tiles, x, maxY, getTile(1, 2));
    }
  }
}

void WorldRenderer::paintRoadNodeOnTiles(const data::RoadGraph::Node& node, const glm::ivec2& position,
                                         std::vector<GLfloat>& tiles) {
  int minX = node.position.getLocal(position).x;
  int minY = node.position.getLocal(position).y;
  int maxX = minX + node.size.x - 1;
  int maxY = minY + node.size.y - 1;

  if (engine.getSettings().rendering.renderRoadNodesAsMarkers) {
    for (int x = minX; x <= maxX; x++) {
      for (int y = minY; y <= maxY; y++) {
        setTile(tiles, x, y, getTile(3, 2));
      }
    }
    return;
  }

  unsigned int tile = getTile(3, 2);

  if (node.isIntersection()) {
    // minX, minY
    if (node.hasS && node.hasE) {
      tile = getTile(3, 0);
    } else if (node.hasS) {
      tile = getTile(0, 1);
    } else if (node.hasE) {
      tile = getTile(1, 0);
    } else {
      tile = getTile(0, 0);
    }
    setTile(tiles, minX, minY, tile);

    // minX, maxY
    if (node.hasN && node.hasE) {
      tile = getTile(3, 1);
    } else if (node.hasN) {
      tile = getTile(0, 1);
    } else if (node.hasE) {
      tile = getTile(1, 2);
    } else {
      tile = getTile(0, 2);
    }
    setTile(tiles, minX, maxY, tile);

    // maxX, minY
    if (node.hasS && node.hasW) {
      tile = getTile(4, 0);
    } else if (node.hasS) {
      tile = getTile(2, 1);
    } else if (node.hasW) {
      tile = getTile(1, 0);
    } else {
      tile = getTile(2, 0);
    }
    setTile(tiles, maxX, minY, tile);

    // maxX, maxY
    if (node.hasN && node.hasW) {
      tile = getTile(4, 1);
    } else if (node.hasN) {
      tile = getTile(2, 1);
    } else if (node.hasW) {
      tile = getTile(1, 2);
    } else {
      tile = getTile(2, 2);
    }
    setTile(tiles, maxX, maxY, tile);
  }

  if (!node.isIntersection()) {
    if (node.size.x > node.size.y) {
      if (node.hasN && node.hasS) {
        // Should never happen unless we cover chunk borders
        setTile(tiles, maxX, maxY, getTile(2, 1));
        setTile(tiles, minX, maxY, getTile(0, 1));
      } else if (node.hasN) {
        setTile(tiles, maxX, maxY, getTile(2, 0));
        setTile(tiles, minX, maxY, getTile(0, 0));
      } else {
        setTile(tiles, maxX, maxY, getTile(2, 2));
        setTile(tiles, minX, maxY, getTile(0, 2));
      }
    } else {
      if (node.hasW && node.hasE) {
        // Should never happen unless we cover chunk borders
        setTile(tiles, maxX, maxY, getTile(1, 2));
        setTile(tiles, maxX, minY, getTile(1, 2));
      } else if (node.hasW) {
        setTile(tiles, maxX, maxY, getTile(0, 2));
        setTile(tiles, maxX, minY, getTile(0, 0));
      } else {
        setTile(tiles, maxX, maxY, getTile(2, 2));
        setTile(tiles, maxX, minY, getTile(2, 0));
      }
    }
  }
}
}