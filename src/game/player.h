//
// Created by Runner on 26.06.2026.
//

#ifndef SHMUP_PLAYER_H
#define SHMUP_PLAYER_H

#include "raylib.h"
#include "shoot/world.h"

typedef struct Player {
    Vector3 position;
    float speed;
    float eyeHeight;
    float radius;
    float height;
    float verticalVelocity;
    float yaw;
    float pitch;
    int health;
    int maxHealth;
    bool grounded;
} Player;

Player Player_Create(Vector3 position);
void Player_Update(Player *player, Camera3D *camera, const World *world);
void Player_TakeDamage(Player *player, int damage);

#endif
