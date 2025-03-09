#include "../include/entity.h"
#include "../include/logging.h"
#include "../include/debug_hud.h"
#include <stdlib.h>
#include <string.h>

// Create a player entity
Entity* entity_create_player(float x, float y) {
    Entity* player = (Entity*)malloc(sizeof(Entity));
    if (!player) {
        LOG_ERROR(LOG_CATEGORY_GAME, "Failed to allocate memory for player entity");
        return NULL;
    }
    
    // Clear the entity structure
    memset(player, 0, sizeof(Entity));
    
    // Set entity type
    player->type = ENTITY_TYPE_PLAYER;
    
    // Set common properties
    player->position_x = x;
    player->position_y = y;
    player->velocity_x = 0.0f;
    player->velocity_y = 0.0f;
    player->width = 1.0f;  // 1 meter wide
    player->height = 2.0f; // 2 meters tall
    
    // Set physics properties
    player->mass = 70.0f;  // 70 kg
    player->is_grounded = false;
    player->is_jumping = false;
    player->is_static = false;
    
    // Set player-specific properties
    player->player.is_moving_left = false;
    player->player.is_moving_right = false;
    player->player.jump_force = 10.0f;
    player->player.move_speed = 20.0f;
    
    // Set state flags
    player->is_active = true;
    player->is_visible = true;
    player->marked_for_deletion = false;
    
    LOG_INFO(LOG_CATEGORY_GAME, "Created player entity at (%.2f, %.2f)", x, y);
    
    return player;
}

// Create an enemy entity
Entity* entity_create_enemy(float x, float y, float patrol_start, float patrol_end) {
    Entity* enemy = (Entity*)malloc(sizeof(Entity));
    if (!enemy) {
        LOG_ERROR(LOG_CATEGORY_GAME, "Failed to allocate memory for enemy entity");
        return NULL;
    }
    
    // Clear the entity structure
    memset(enemy, 0, sizeof(Entity));
    
    // Set entity type
    enemy->type = ENTITY_TYPE_ENEMY;
    
    // Get initial enemy speed from debug HUD
    float patrol_speed = debug_hud_get_enemy_speed();
    
    // Log the initial speed
    LOG_INFO(LOG_CATEGORY_GAME, "Creating enemy with patrol speed: %.1f m/s", patrol_speed);
    
    // Set common properties
    enemy->position_x = x;
    enemy->position_y = y;
    enemy->velocity_x = patrol_speed;  // Use the patrol speed from debug HUD
    enemy->velocity_y = 0.0f;
    enemy->width = 1.0f;  // 1 meter wide
    enemy->height = 2.0f; // 2 meters tall
    
    // Set physics properties
    enemy->mass = 70.0f;  // 70 kg
    enemy->is_grounded = false;
    enemy->is_jumping = false;
    enemy->is_static = false;
    
    // Set enemy-specific properties
    enemy->enemy.patrol_start_x = patrol_start;
    enemy->enemy.patrol_end_x = patrol_end;
    enemy->enemy.patrol_speed = patrol_speed;  // Initialize patrol speed
    enemy->enemy.aggro_range = 5.0f;
    enemy->enemy.attack_range = 1.5f;
    enemy->enemy.is_aggressive = true;
    enemy->enemy.health = 100;
    
    // Set state flags
    enemy->is_active = true;
    enemy->is_visible = true;
    enemy->marked_for_deletion = false;
    
    LOG_INFO(LOG_CATEGORY_GAME, "Created enemy entity at (%.2f, %.2f) with patrol range (%.2f, %.2f)", 
           x, y, patrol_start, patrol_end);
    
    return enemy;
}

// Update an entity based on its type
void entity_update(Entity* entity, double dt) {
    if (!entity || !entity->is_active) {
        return;
    }
    
    // Type-specific updates
    switch (entity->type) {
        case ENTITY_TYPE_PLAYER:
            // Player-specific update logic would go here
            // This will be handled by the physics system
            break;
            
        case ENTITY_TYPE_ENEMY:
            // Simple enemy patrol AI
            if (entity->position_x <= entity->enemy.patrol_start_x) {
                entity->velocity_x = 2.0f;  // Move right
            } else if (entity->position_x >= entity->enemy.patrol_end_x) {
                entity->velocity_x = -2.0f; // Move left
            }
            break;
            
        case ENTITY_TYPE_PROP:
            // Props don't need special updates
            break;
            
        default:
            break;
    }
}

// Destroy an entity and free its memory
void entity_destroy(Entity* entity) {
    if (entity) {
        LOG_INFO(LOG_CATEGORY_GAME, "Destroying entity of type %d at (%.2f, %.2f)", 
               entity->type, entity->position_x, entity->position_y);
        free(entity);
    }
} 