#include "Map.hpp"

namespace world {
Map::Map() {
  buildingCount = 0;
}

void Map::cleanup() {
  for (auto it = chunks.begin(); chunks.end() != it; it++) {
    delete *it;
  }
  chunks.clear();
}

void Map::createChunk(glm::ivec2 position) {
  // TODO(kantoniak): Check if chunk exists already
  data::Chunk* chunk = new data::Chunk;
  chunk->setPosition(position);
  chunks.push_back(chunk);
}

void Map::randomizeChunk(glm::ivec2 position) {
  data::Chunk* chunk = nullptr;
  for (data::Chunk* c : getChunks()) {
    if (glm::all(glm::equal(position, c->getPosition()))) {
      chunk = c;
      break;
    }
  }
  if (chunk == nullptr) {
    return;
  }
  chunk->randomizeBuildings();
  buildingCount += chunk->getResidentialSize();
}

unsigned int Map::getChunksCount() {
  return chunks.size();
}

Map::chunkList Map::getChunks() {
  return chunks;
}

bool Map::chunkExists(glm::ivec2 chunkPosition) {
  for (data::Chunk* chunk : getChunks()) {
    if (glm::all(glm::equal(chunk->getPosition(), chunkPosition))) {
      return true;
    }
  }
  return false;
}

Map::chunkListIter Map::getChunkIterator() {
  return chunks.begin();
}

void Map::addBuilding(data::buildings::Building building) {
  glm::ivec2 chunk = glm::ivec2(building.x, building.y) / (int)data::Chunk::SIDE_LENGTH;
  if (chunkExists(chunk)) {
    getChunk(chunk).addBuilding(building);
    buildingCount++;
  }
}

unsigned int Map::getBuildingCount() {
  return buildingCount;
}

data::Chunk& Map::getChunk(glm::ivec2 chunkPosition) {
  for (unsigned long it = 0; it < chunks.size(); it++) {
    if (glm::all(glm::equal(chunks[it]->getPosition(), chunkPosition))) {
      return *(chunks[it]);
    }
  }
  throw std::invalid_argument("Chunk does not exist");
}
}