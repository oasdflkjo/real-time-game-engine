#ifndef ENTITY_H
#define ENTITY_H

#include <stdbool.h>

// Entity types
typedef enum {
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_ENEMY,
    ENTITY_TYPE_PROP,
    // Add more entity types as needed
    ENTITY_TYPE_COUNT
} EntityType;

// Base entity structure
typedef struct {
    // Entity type
    EntityType type;
    
    // Common properties
    float position_x;
    float position_y;
    float velocity_x;
    float velocity_y;
    float width;
    float height;
    
    // Physics properties
    float mass;
    bool is_grounded;
    bool is_jumping;
    bool is_static;  // Static entities don't move (like platforms)
    
    // Visual properties
    unsigned int texture_id;  // OpenGL texture ID
    
    // Entity-specific properties
    union {
        // Player-specific data
        struct {
            bool is_moving_left;
            bool is_moving_right;
            float jump_force;
            float move_speed;
        } player;
        
        // Enemy-specific data
        struct {
            float patrol_start_x;
            float patrol_end_x;
            float patrol_speed;
            float aggro_range;
            float attack_range;
            bool is_aggressive;
            int health;
        } enemy;
        
        // Add more entity-specific data as needed
    };
    
    // Entity state flags
    bool is_active;
    bool is_visible;
    bool marked_for_deletion;
} Entity;

// Entity creation functions
Entity* entity_create_player(float x, float y);
Entity* entity_create_enemy(float x, float y, float patrol_start, float patrol_end);

// Entity management functions
void entity_update(Entity* entity, double dt);
void entity_destroy(Entity* entity);

#endif // ENTITY_H 