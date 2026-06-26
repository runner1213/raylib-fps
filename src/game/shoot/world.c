//
// Created by Runner on 26.06.2026.
//

#include "world.h"
#include "raymath.h"

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

World World_Create(void) {
    World world = { 0 };

    world.obstacleCenters[0] = (Vector3){ 3.0f, 1.0f, 3.0f };
    world.obstacleSizes[0] = (Vector3){ 2.0f, 2.0f, 2.0f };
    world.obstacleColors[0] = RED;

    world.obstacleCenters[1] = (Vector3){ -4.0f, 1.0f, -2.0f };
    world.obstacleSizes[1] = (Vector3){ 2.0f, 2.0f, 2.0f };
    world.obstacleColors[1] = BLUE;

    world.obstacleCenters[2] = (Vector3){ 0.0f, 1.0f, -8.0f };
    world.obstacleSizes[2] = (Vector3){ 2.0f, 2.0f, 2.0f };
    world.obstacleColors[2] = GREEN;

    for (int i = 0; i < WORLD_OBSTACLE_COUNT; i++) {
        world.obstacles[i] = MakeBox(world.obstacleCenters[i], world.obstacleSizes[i]);
    }

    return world;
}

void World_Update(World *world, float dt) {
    for (int i = 0; i < WORLD_BULLET_MARK_COUNT; i++) {
        if (world->bulletMarks[i].life > 0.0f) {
            world->bulletMarks[i].life -= dt;
        }
    }
}

void World_Draw(const World *world) {
    DrawPlane((Vector3){ 0, 0, 0 }, (Vector2){ 30, 30 }, LIGHTGRAY);
    DrawGrid(30, 1.0f);

    for (int i = 0; i < WORLD_OBSTACLE_COUNT; i++) {
        DrawCubeV(world->obstacleCenters[i], world->obstacleSizes[i], world->obstacleColors[i]);
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

    for (int i = 0; i < WORLD_OBSTACLE_COUNT; i++) {
        const RayCollision hit = GetRayCollisionBox(ray, world->obstacles[i]);

        if (hit.hit && hit.distance < bestHit.distance) {
            bestHit = hit;
        }
    }

    if (bestHit.hit) {
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
