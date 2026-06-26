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
    bool grounded;
} Player;

Player Player_Create(Vector3 position);
void Player_Update(Player *player, Camera3D *camera, const World *world);

#endif
