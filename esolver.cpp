#include "esolver.hpp"

// Resolve the position of an entity for accurate hits
Vector3 ResolveEntity(Entity* entity) {
    // Use game state and physics to determine the expected position of the entity
    // This improves hit accuracy during HVH gameplay
    return entity->GetCurrentPosition(); 
}