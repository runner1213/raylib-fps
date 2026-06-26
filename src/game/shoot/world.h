//
// Created by Runner on 26.06.2026.
//

#ifndef SHMUP_WORLD_H
#define SHMUP_WORLD_H

#include "raylib.h"
#include "weapon.h"

#define WORLD_OBSTACLE_COUNT 3
#define WORLD_BULLET_MARK_COUNT 64

typedef struct World {
    BoundingBox obstacles[WORLD_OBSTACLE_COUNT];
    Vector3 obstacleCenters[WORLD_OBSTACLE_COUNT];
    Vector3 obstacleSizes[WORLD_OBSTACLE_COUNT];
    Color obstacleColors[WORLD_OBSTACLE_COUNT];
    BulletMark bulletMarks[WORLD_BULLET_MARK_COUNT];
    int nextBulletMark;
} World;

World World_Create(void);
void World_Update(World *world, float dt);
void World_Draw(const World *world);
void World_Shoot(World *world, Camera3D camera);
void World_DrawBulletMarks(const World *world);
bool World_CheckPlayerCollision(const World *world, Vector3 position, float radius, float height);
float World_GetGroundHeight(const World *world, Vector3 position, float radius);

#endif //SHMUP_WORLD_H
