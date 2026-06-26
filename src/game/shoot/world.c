//
// Created by Runner on 26.06.2026.
//

#include "world.h"
#include "raymath.h"

#include <math.h>

static BoundingBox MakeBox(Vector3 center, Vector3 size) {
    const Vector3 half = Vector3Scale(size, 0.5f);

    return (BoundingBox){
        .min = Vector3Subtract(center, half),
        .max = Vector3Add(center, half)
    };
}

static Vector3 ChooseTangent(Vector3 normal) {
    Vector3 tangent = Vector3CrossProduct(normal, (Vector3){ 0.0f, 1.0f, 0.0f });

    if (Vector3LengthSqr(tangent) < 0.001f) {
        tangent = Vector3CrossProduct(normal, (Vector3){ 1.0f, 0.0f, 0.0f });
    }

    return Vector3Normalize(tangent);
}

static BoundingBox EnemyBox(Enemy enemy) {
    return MakeBox(enemy.position, enemy.size);
}

static void SpawnAmmoPickup(World *world, Vector3 position) {
    world->ammoPickups[world->nextAmmoPickup] = (AmmoPickup){
        .position = (Vector3){ position.x, 0.22f, position.z },
        .size = (Vector3){ 0.55f, 0.22f, 0.32f },
        .amount = 5,
        .active = true
    };
    world->nextAmmoPickup = (world->nextAmmoPickup + 1) % WORLD_AMMO_PICKUP_COUNT;
}

static int ActiveEnemyCount(const World *world) {
    int count = 0;

    for (int i = 0; i < WORLD_ENEMY_COUNT; i++) {
        if (world->enemies[i].active) {
            count++;
        }
    }

    return count;
}

static void SpawnWave(World *world, Vector3 center) {
    int count = world->waveSize;

    if (count > WORLD_ENEMY_COUNT) {
        count = WORLD_ENEMY_COUNT;
    }

    for (int i = 0; i < WORLD_ENEMY_COUNT; i++) {
        world->enemies[i].active = false;
    }

    for (int i = 0; i < count; i++) {
        const float angle = ((float)i / (float)count) * PI * 2.0f;
        const float ring = 8.0f + (float)(i % 4) * 1.8f;
        const Vector3 position = {
            center.x + cosf(angle) * ring,
            0.45f,
            center.z + sinf(angle) * ring
        };

        world->enemies[i] = (Enemy){
            .position = position,
            .size = (Vector3){ 0.9f, 0.9f, 0.9f },
            .speed = 1.85f + 0.05f * (float)world->waveNumber,
            .damageCooldown = 0.0f,
            .active = true
        };
    }

    world->waveNumber++;
    world->waveSize *= 2;
}

World World_Create(void) {
    World world = { 0 };

    world.obstacleCenters[0] = (Vector3){ 3.0f, 1.0f, 3.0f };
    world.obstacleSizes[0] = (Vector3){ 2.0f, 2.0f, 2.0f };
    world.obstacleColors[0] = DARKGRAY;

    world.obstacleCenters[1] = (Vector3){ -4.0f, 1.0f, -2.0f };
    world.obstacleSizes[1] = (Vector3){ 2.0f, 2.0f, 2.0f };
    world.obstacleColors[1] = BLUE;

    world.obstacleCenters[2] = (Vector3){ 0.0f, 1.0f, -8.0f };
    world.obstacleSizes[2] = (Vector3){ 2.0f, 2.0f, 2.0f };
    world.obstacleColors[2] = GREEN;

    for (int i = 0; i < WORLD_OBSTACLE_COUNT; i++) {
        world.obstacles[i] = MakeBox(world.obstacleCenters[i], world.obstacleSizes[i]);
    }

    world.waveNumber = 1;
    world.waveSize = 5;
    SpawnWave(&world, (Vector3){ 0.0f, 0.0f, 0.0f });

    return world;
}

void World_Update(World *world, float dt, Vector3 playerPosition) {
    for (int i = 0; i < WORLD_BULLET_MARK_COUNT; i++) {
        if (world->bulletMarks[i].life > 0.0f) {
            world->bulletMarks[i].life -= dt;
        }
    }

    for (int i = 0; i < WORLD_ENEMY_COUNT; i++) {
        Enemy *enemy = &world->enemies[i];

        if (!enemy->active) {
            continue;
        }

        if (enemy->damageCooldown > 0.0f) {
            enemy->damageCooldown -= dt;
        }

        Vector3 toPlayer = Vector3Subtract(playerPosition, enemy->position);
        toPlayer.y = 0.0f;

        if (Vector3LengthSqr(toPlayer) > 0.001f) {
            const Vector3 direction = Vector3Normalize(toPlayer);
            Vector3 nextPosition = enemy->position;
            nextPosition.x += direction.x * enemy->speed * dt;
            nextPosition.z += direction.z * enemy->speed * dt;
            enemy->position = nextPosition;
        }
    }

    if (ActiveEnemyCount(world) == 0) {
        SpawnWave(world, playerPosition);
    }
}

void World_Draw(const World *world) {
    DrawPlane((Vector3){ 0, 0, 0 }, (Vector2){ 30, 30 }, LIGHTGRAY);
    DrawGrid(30, 1.0f);

    for (int i = 0; i < WORLD_OBSTACLE_COUNT; i++) {
        DrawCubeV(world->obstacleCenters[i], world->obstacleSizes[i], world->obstacleColors[i]);
    }

    for (int i = 0; i < WORLD_ENEMY_COUNT; i++) {
        const Enemy enemy = world->enemies[i];

        if (enemy.active) {
            DrawCubeV(enemy.position, enemy.size, RED);
        }
    }

    for (int i = 0; i < WORLD_AMMO_PICKUP_COUNT; i++) {
        const AmmoPickup pickup = world->ammoPickups[i];

        if (pickup.active) {
            DrawCubeV(pickup.position, pickup.size, (Color){ 112, 68, 38, 255 });
        }
    }
}

void World_Shoot(World *world, Camera3D camera) {
    const Vector3 direction = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    const Ray ray = {
        .position = camera.position,
        .direction = direction
    };

    RayCollision bestHit = { 0 };
    bestHit.distance = 1000000.0f;

    int hitEnemy = -1;

    for (int i = 0; i < WORLD_ENEMY_COUNT; i++) {
        if (!world->enemies[i].active) {
            continue;
        }

        const RayCollision hit = GetRayCollisionBox(ray, EnemyBox(world->enemies[i]));

        if (hit.hit && hit.distance < bestHit.distance) {
            bestHit = hit;
            hitEnemy = i;
        }
    }

    for (int i = 0; i < WORLD_OBSTACLE_COUNT; i++) {
        const RayCollision hit = GetRayCollisionBox(ray, world->obstacles[i]);

        if (hit.hit && hit.distance < bestHit.distance) {
            bestHit = hit;
            hitEnemy = -1;
        }
    }

    if (bestHit.hit) {
        if (hitEnemy >= 0) {
            const Vector3 enemyPosition = world->enemies[hitEnemy].position;
            world->enemies[hitEnemy].active = false;
            SpawnAmmoPickup(world, enemyPosition);
            return;
        }

        world->bulletMarks[world->nextBulletMark] = (BulletMark){
            .position = Vector3Add(bestHit.point, Vector3Scale(bestHit.normal, 0.012f)),
            .normal = bestHit.normal,
            .size = 0.18f,
            .life = 18.0f
        };
        world->nextBulletMark = (world->nextBulletMark + 1) % WORLD_BULLET_MARK_COUNT;
    }
}

void World_DrawBulletMarks(const World *world) {
    for (int i = 0; i < WORLD_BULLET_MARK_COUNT; i++) {
        const BulletMark mark = world->bulletMarks[i];

        if (mark.life > 0.0f) {
            const Vector3 tangent = ChooseTangent(mark.normal);
            const Vector3 bitangent = Vector3Normalize(Vector3CrossProduct(mark.normal, tangent));
            const float half = mark.size * 0.5f;
            const float alpha = Clamp(mark.life / 2.0f, 0.0f, 1.0f);
            const Color color = Fade((Color){ 70, 70, 70, 255 }, alpha);

            const Vector3 p0 = Vector3Add(Vector3Add(mark.position, Vector3Scale(tangent, -half)), Vector3Scale(bitangent, -half));
            const Vector3 p1 = Vector3Add(Vector3Add(mark.position, Vector3Scale(tangent, half)), Vector3Scale(bitangent, -half));
            const Vector3 p2 = Vector3Add(Vector3Add(mark.position, Vector3Scale(tangent, half)), Vector3Scale(bitangent, half));
            const Vector3 p3 = Vector3Add(Vector3Add(mark.position, Vector3Scale(tangent, -half)), Vector3Scale(bitangent, half));

            DrawTriangle3D(p0, p1, p2, color);
            DrawTriangle3D(p0, p2, p3, color);
        }
    }
}

bool World_CheckPlayerCollision(const World *world, Vector3 position, float radius, float height) {
    const float playerMinY = position.y;
    const float playerMaxY = position.y + height;

    for (int i = 0; i < WORLD_OBSTACLE_COUNT; i++) {
        const BoundingBox box = world->obstacles[i];

        if (playerMaxY <= box.min.y || playerMinY >= box.max.y) {
            continue;
        }

        const float closestX = Clamp(position.x, box.min.x, box.max.x);
        const float closestZ = Clamp(position.z, box.min.z, box.max.z);
        const float dx = position.x - closestX;
        const float dz = position.z - closestZ;

        if ((dx * dx + dz * dz) < (radius * radius)) {
            return true;
        }
    }

    return false;
}

float World_GetGroundHeight(const World *world, Vector3 position, float radius) {
    float groundHeight = 0.0f;

    for (int i = 0; i < WORLD_OBSTACLE_COUNT; i++) {
        const BoundingBox box = world->obstacles[i];
        const float closestX = Clamp(position.x, box.min.x, box.max.x);
        const float closestZ = Clamp(position.z, box.min.z, box.max.z);
        const float dx = position.x - closestX;
        const float dz = position.z - closestZ;

        if ((dx * dx + dz * dz) < (radius * radius) && box.max.y > groundHeight) {
            groundHeight = box.max.y;
        }
    }

    return groundHeight;
}

int World_GetEnemyDamage(World *world, Vector3 playerPosition, float playerRadius) {
    int damage = 0;

    for (int i = 0; i < WORLD_ENEMY_COUNT; i++) {
        Enemy *enemy = &world->enemies[i];

        if (!enemy->active || enemy->damageCooldown > 0.0f) {
            continue;
        }

        const float attackRadius = playerRadius + enemy->size.x * 0.75f;
        const float dx = playerPosition.x - enemy->position.x;
        const float dz = playerPosition.z - enemy->position.z;

        if ((dx * dx + dz * dz) <= attackRadius * attackRadius) {
            damage += 10;
            enemy->damageCooldown = 1.0f;
        }
    }

    return damage;
}

int World_CollectAmmo(World *world, Vector3 playerPosition, float playerRadius) {
    int collectedAmmo = 0;

    for (int i = 0; i < WORLD_AMMO_PICKUP_COUNT; i++) {
        AmmoPickup *pickup = &world->ammoPickups[i];

        if (!pickup->active) {
            continue;
        }

        const float pickupRadius = playerRadius + 0.55f;
        const float dx = playerPosition.x - pickup->position.x;
        const float dz = playerPosition.z - pickup->position.z;

        if ((dx * dx + dz * dz) <= pickupRadius * pickupRadius) {
            collectedAmmo += pickup->amount;
            pickup->active = false;
        }
    }

    return collectedAmmo;
}

int World_GetCurrentWave(const World *world) {
    return world->waveNumber - 1;
}
