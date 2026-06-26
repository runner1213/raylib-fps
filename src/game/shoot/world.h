//
// Created by Runner on 26.06.2026.
//

#ifndef SHMUP_WORLD_H
#define SHMUP_WORLD_H

#include "raylib.h"
#include "weapon.h"

#define WORLD_OBSTACLE_COUNT 3
#define WORLD_BULLET_MARK_COUNT 64
#define WORLD_ENEMY_COUNT 128
#define WORLD_AMMO_PICKUP_COUNT 128

typedef struct Enemy {
    Vector3 position;
    Vector3 size;
    float speed;
    float damageCooldown;
    bool active;
} Enemy;

typedef struct AmmoPickup {
    Vector3 position;
    Vector3 size;
    int amount;
    bool active;
} AmmoPickup;

typedef struct World {
    BoundingBox obstacles[WORLD_OBSTACLE_COUNT];
    Vector3 obstacleCenters[WORLD_OBSTACLE_COUNT];
    Vector3 obstacleSizes[WORLD_OBSTACLE_COUNT];
    Color obstacleColors[WORLD_OBSTACLE_COUNT];
    Enemy enemies[WORLD_ENEMY_COUNT];
    AmmoPickup ammoPickups[WORLD_AMMO_PICKUP_COUNT];
    BulletMark bulletMarks[WORLD_BULLET_MARK_COUNT];
    int nextBulletMark;
    int nextAmmoPickup;
    int waveNumber;
    int waveSize;
} World;

World World_Create(void);
void World_Update(World *world, float dt, Vector3 playerPosition);
void World_Draw(const World *world);
void World_Shoot(World *world, Camera3D camera);
void World_DrawBulletMarks(const World *world);
bool World_CheckPlayerCollision(const World *world, Vector3 position, float radius, float height);
float World_GetGroundHeight(const World *world, Vector3 position, float radius);
int World_GetEnemyDamage(World *world, Vector3 playerPosition, float playerRadius);
int World_CollectAmmo(World *world, Vector3 playerPosition, float playerRadius);
int World_GetCurrentWave(const World *world);

#endif //SHMUP_WORLD_H
