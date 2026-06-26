//
// Created by Runner on 26.06.2026.
//

#include "player.h"
#include "raymath.h"

Player Player_Create(Vector3 position) {
    return (Player){
        .position = position,
        .speed = 10.0f,
        .eyeHeight = 2.0f,
        .radius = 0.35f,
        .height = 2.0f,
        .verticalVelocity = 0.0f,
        .yaw = -90.0f,
        .pitch = 0.0f,
        .health = 100,
        .maxHealth = 100,
        .grounded = true
    };
}

void Player_Update(Player *player, Camera3D *camera, const World *world) {
    const float dt = GetFrameTime();
    const float mouseSensitivity = 0.1f;
    const float gravity = 24.0f;
    const float jumpSpeed = 12.0f;

    Vector2 mouseDelta = GetMouseDelta();

    player->yaw += mouseDelta.x * mouseSensitivity;
    player->pitch -= mouseDelta.y * mouseSensitivity;

    if (player->pitch > 89.0f) player->pitch = 89.0f;
    if (player->pitch < -89.0f) player->pitch = -89.0f;

    float yawRad = DEG2RAD * player->yaw;
    float pitchRad = DEG2RAD * player->pitch;

    Vector3 forward = {
        cosf(yawRad) * cosf(pitchRad),
        sinf(pitchRad),
        sinf(yawRad) * cosf(pitchRad)
    };

    forward = Vector3Normalize(forward);

    Vector3 right = Vector3Normalize(
        Vector3CrossProduct(forward, (Vector3){ 0, 1, 0 })
    );

    Vector3 move = { 0 };

    if (IsKeyDown(KEY_W)) move = Vector3Add(move, forward);
    if (IsKeyDown(KEY_S)) move = Vector3Subtract(move, forward);
    if (IsKeyDown(KEY_D)) move = Vector3Add(move, right);
    if (IsKeyDown(KEY_A)) move = Vector3Subtract(move, right);

    move.y = 0.0f;

    if (Vector3Length(move) > 0.0f) {
        move = Vector3Normalize(move);
        move = Vector3Scale(move, player->speed * dt);

        Vector3 nextPosition = player->position;
        nextPosition.x += move.x;

        if (!World_CheckPlayerCollision(world, nextPosition, player->radius, player->height)) {
            player->position.x = nextPosition.x;
        }

        nextPosition = player->position;
        nextPosition.z += move.z;

        if (!World_CheckPlayerCollision(world, nextPosition, player->radius, player->height)) {
            player->position.z = nextPosition.z;
        }
    }

    if (player->grounded && IsKeyPressed(KEY_SPACE)) {
        player->verticalVelocity = jumpSpeed;
        player->grounded = false;
    }

    player->verticalVelocity -= gravity * dt;
    player->position.y += player->verticalVelocity * dt;

    const float groundHeight = World_GetGroundHeight(world, player->position, player->radius);

    if (player->position.y <= groundHeight) {
        player->position.y = groundHeight;
        player->verticalVelocity = 0.0f;
        player->grounded = true;
    } else {
        player->grounded = false;
    }

    camera->position = (Vector3){
        player->position.x,
        player->position.y + player->eyeHeight,
        player->position.z
    };

    camera->target = Vector3Add(camera->position, forward);
}

void Player_TakeDamage(Player *player, int damage) {
    player->health -= damage;

    if (player->health < 0) {
        player->health = 0;
    }
}
