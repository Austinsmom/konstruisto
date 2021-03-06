#include "Chunk.hpp"

namespace data {

Chunk::Chunk() {
  objectId = 0;
  position = glm::ivec2();
  residentialSize = 0;
}

void Chunk::setObjectId(unsigned int objectId) {
  this->objectId = objectId;
}

void Chunk::setPosition(glm::ivec2 position) {
  this->position = position;
  if (position == glm::ivec2(0, 0)) {
    roadGraph.test();
  }
}

glm::ivec2 Chunk::getPosition() const {
  return position;
}

Chunk::residentialList Chunk::getResidentials() const {
  return residential;
}

Chunk::residentialListIter Chunk::getResidentialIterator() const {
  return residential.begin();
}

unsigned int Chunk::getResidentialSize() const {
  return residentialSize;
}

void Chunk::addLot(data::Lot lot) {
  lots.push_back(lot);
}

Chunk::lotList Chunk::getLots() const {
  return lots;
}

void Chunk::addBuilding(data::buildings::Building building) {
  residential.push_back(building);
}

bool Chunk::removeBuilding(data::buildings::Building building) {
  auto toRemove = residential.end();
  for (auto it = residential.begin(); it != residential.end(); it++) {
    if ((*it).x == building.x && (*it).y == building.y) {
      toRemove = it;
    }
  }
  if (toRemove == residential.end()) {
    return false;
  }
  residential.erase(toRemove);
  return true;
}

void Chunk::addRoad(Road road) {
  roadGraph.addRoad(road);
}

const std::vector<Road>& Chunk::getRoads() const {
  return roadGraph.getRoads();
}

const RoadGraph& Chunk::getRoadGraph() const {
  return roadGraph;
}
}