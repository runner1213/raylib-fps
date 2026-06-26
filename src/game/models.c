//
// Created by Runner on 26.06.2026.
//

#include "models.h"
#include "raylib.h"
#include "raymath.h"
#include "smoke_assets.h"

#include <math.h>
#include <stddef.h>

#define SMOKE_PARTICLE_COUNT 12
#define TRACER_COUNT 8
#define AK_MAGAZINE_SIZE 30
#define AK_RELOAD_TIME 3.0f

typedef struct SmokeParticle {
    Vector3 position;
    Vector3 velocity;
    float life;
    float maxLife;
    float size;
    float growth;
    float rotation;
    float spin;
    float drift;
} SmokeParticle;

typedef struct BulletTracer {
    Vector3 start;
    Vector3 end;
    float life;
    float maxLife;
} BulletTracer;

static SmokeParticle smokeParticles[SMOKE_PARTICLE_COUNT];
static BulletTracer tracers[TRACER_COUNT];
static int nextSmokeParticle = 0;
static int nextTracer = 0;
static float aimAmount = 0.0f;
static float fireCooldown = 0.0f;
static float flashTimer = 0.0f;
static float recoil = 0.0f;
static float reloadTimer = 0.0f;
static Texture2D smokeTexture = { 0 };
static bool shotThisFrame = false;
static int magazineAmmo = AK_MAGAZINE_SIZE;
static int reserveAmmo = 90;

static float Approach(float current, float target, float speed) {
    if (current < target) {
        current += speed;
        if (current > target) current = target;
    } else {
        current -= speed;
        if (current < target) current = target;
    }

    return current;
}

static Vector3 Lerp3(Vector3 a, Vector3 b, float t) {
    return (Vector3){
        Lerp(a.x, b.x, t),
        Lerp(a.y, b.y, t),
        Lerp(a.z, b.z, t)
    };
}

static Texture2D GetSmokeTexture(void) {
    if (smokeTexture.id == 0) {
        Image smokeImage = { 0 };

        if (HAS_SMOKE_TEXTURE) {
            smokeImage = LoadImageFromMemory(".png", SMOKE_TEXTURE_DATA, SMOKE_TEXTURE_SIZE);
        }

        if (smokeImage.data == NULL) {
            smokeImage = GenImageGradientRadial(
                96,
                96,
                0.18f,
                (Color){ 230, 230, 230, 190 },
                (Color){ 120, 120, 120, 0 }
            );
            ImageBlurGaussian(&smokeImage, 10);
        }

        smokeTexture = LoadTextureFromImage(smokeImage);
        UnloadImage(smokeImage);
    }

    return smokeTexture;
}

static Matrix MakeWeaponTransform(Vector3 forward, Vector3 right, Vector3 up, float roll) {
    const Vector3 xAxis = forward;
    const Vector3 yAxis = up;
    const Vector3 zAxis = Vector3Scale(right, -1.0f);

    return (Matrix){
        xAxis.x, yAxis.x, zAxis.x, 0.0f,
        xAxis.y, yAxis.y, zAxis.y, 0.0f,
        xAxis.z, yAxis.z, zAxis.z, 0.0f,
        0.0f,   0.0f,   0.0f,   1.0f
    };
}

static void ApplyAkMaterialColors(Model *weapon) {
    static const Color materialColors[] = {
        { 28, 29, 28, 255 },
        { 164, 160, 150, 255 },
        { 86, 50, 32, 255 },
        { 24, 24, 24, 255 },
        { 38, 38, 37, 255 },
        { 22, 22, 22, 255 },
        { 46, 46, 44, 255 },
        { 20, 20, 20, 255 },
        { 58, 57, 52, 255 },
        { 30, 30, 29, 255 },
        { 102, 58, 36, 255 }
    };

    const int colorCount = sizeof(materialColors) / sizeof(materialColors[0]);

    for (int i = 0; i < weapon->materialCount; i++) {
        const int colorIndex = (i < colorCount) ? i : 0;
        weapon->materials[i].maps[MATERIAL_MAP_DIFFUSE].color = materialColors[colorIndex];
    }
}

static void SpawnShotEffects(Vector3 muzzlePos, Vector3 forward, Vector3 right, Vector3 up) {
    flashTimer = 0.055f;
    recoil = 1.0f;

    tracers[nextTracer] = (BulletTracer){
        .start = muzzlePos,
        .end = Vector3Add(muzzlePos, Vector3Scale(forward, 90.0f)),
        .life = 0.055f,
        .maxLife = 0.055f
    };
    nextTracer = (nextTracer + 1) % TRACER_COUNT;

    for (int i = 0; i < 8; i++) {
        const float side = (float)GetRandomValue(-100, 100) / 100.0f;
        const float lift = (float)GetRandomValue(0, 100) / 100.0f;
        const float push = (float)GetRandomValue(10, 100) / 100.0f;
        const float life = 0.72f + (float)GetRandomValue(0, 45) / 100.0f;

        Vector3 velocity = Vector3Scale(forward, 0.35f * push);
        velocity = Vector3Add(velocity, Vector3Scale(right, side * 0.17f));
        velocity = Vector3Add(velocity, Vector3Scale(up, 0.10f + lift * 0.20f));

        Vector3 position = muzzlePos;
        position = Vector3Add(position, Vector3Scale(forward, (float)i * 0.012f));
        position = Vector3Add(position, Vector3Scale(right, side * 0.025f));
        position = Vector3Add(position, Vector3Scale(up, lift * 0.018f));

        smokeParticles[nextSmokeParticle] = (SmokeParticle){
            .position = position,
            .velocity = velocity,
            .life = life,
            .maxLife = life,
            .size = 0.045f + (float)GetRandomValue(0, 35) / 1000.0f,
            .growth = 0.20f + (float)GetRandomValue(0, 12) / 100.0f,
            .rotation = (float)GetRandomValue(0, 360),
            .spin = (float)GetRandomValue(-80, 80),
            .drift = side
        };
        nextSmokeParticle = (nextSmokeParticle + 1) % SMOKE_PARTICLE_COUNT;
    }
}

static void UpdateShotEffects(float dt) {
    if (fireCooldown > 0.0f) fireCooldown -= dt;
    if (flashTimer > 0.0f) flashTimer -= dt;

    recoil = Approach(recoil, 0.0f, dt * 7.0f);

    for (int i = 0; i < TRACER_COUNT; i++) {
        if (tracers[i].life > 0.0f) {
            tracers[i].life -= dt;
        }
    }

    for (int i = 0; i < SMOKE_PARTICLE_COUNT; i++) {
        SmokeParticle *particle = &smokeParticles[i];

        if (particle->life > 0.0f) {
            particle->life -= dt;
            const float age = 1.0f - particle->life / particle->maxLife;
            const float curl = sinf(age * 8.0f + particle->drift * 3.0f) * 0.08f;

            particle->velocity.x += curl * dt;
            particle->velocity.y += (0.20f + age * 0.18f) * dt;
            particle->velocity.z += curl * particle->drift * dt;
            particle->position = Vector3Add(
                particle->position,
                Vector3Scale(particle->velocity, dt)
            );
            particle->velocity = Vector3Scale(particle->velocity, 0.965f);
            particle->size += particle->growth * dt;
            particle->rotation += particle->spin * dt;
        }
    }
}

static void StartReload(void) {
    if (reloadTimer <= 0.0f && magazineAmmo < AK_MAGAZINE_SIZE && reserveAmmo > 0) {
        reloadTimer = AK_RELOAD_TIME;
    }
}

static void UpdateReload(float dt) {
    if (reloadTimer <= 0.0f) {
        return;
    }

    reloadTimer -= dt;

    if (reloadTimer <= 0.0f) {
        const int neededAmmo = AK_MAGAZINE_SIZE - magazineAmmo;
        const int loadedAmmo = (reserveAmmo < neededAmmo) ? reserveAmmo : neededAmmo;

        magazineAmmo += loadedAmmo;
        reserveAmmo -= loadedAmmo;
        reloadTimer = 0.0f;
    }
}

static void DrawShotEffects(Camera3D camera, Vector3 muzzlePos, Vector3 forward) {
    for (int i = 0; i < TRACER_COUNT; i++) {
        const BulletTracer tracer = tracers[i];

        if (tracer.life > 0.0f) {
            const float alpha = tracer.life / tracer.maxLife;
            DrawLine3D(tracer.start, tracer.end, Fade((Color){ 255, 219, 74, 255 }, alpha));
        }
    }

    const Texture2D texture = GetSmokeTexture();
    const Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };

    for (int i = 0; i < SMOKE_PARTICLE_COUNT; i++) {
        const SmokeParticle particle = smokeParticles[i];

        if (particle.life > 0.0f) {
            const float age = 1.0f - particle.life / particle.maxLife;
            const float fadeIn = fminf(age * 8.0f, 1.0f);
            const float fadeOut = particle.life / particle.maxLife;
            const float alpha = 0.42f * fadeIn * fadeOut;
            const float size = particle.size * (1.0f + age * 0.8f);
            const Color tint = Fade((Color){ 165, 165, 160, 255 }, alpha);

            DrawBillboardPro(
                camera,
                texture,
                source,
                particle.position,
                (Vector3){ 0.0f, 1.0f, 0.0f },
                (Vector2){ size, size },
                (Vector2){ size * 0.5f, size * 0.5f },
                particle.rotation,
                tint
            );
        }
    }

    if (flashTimer > 0.0f) {
        const float alpha = flashTimer / 0.055f;
        const Vector3 flashCenter = Vector3Add(muzzlePos, Vector3Scale(forward, 0.08f));

        DrawSphere(flashCenter, 0.12f * alpha, Fade(ORANGE, alpha));
        DrawSphere(flashCenter, 0.065f * alpha, Fade(YELLOW, alpha));
    }
}

void drawAk(const Model ak, Camera3D camera) {
    const float dt = GetFrameTime();
    shotThisFrame = false;

    const Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    const Vector3 worldUp = { 0.0f, 1.0f, 0.0f };
    const Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, worldUp));
    const Vector3 up = Vector3Normalize(Vector3CrossProduct(right, forward));

    UpdateReload(dt);

    if (IsKeyPressed(KEY_R)) {
        StartReload();
    }

    const bool isReloading = reloadTimer > 0.0f;
    const float aimTarget = (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && !isReloading) ? 1.0f : 0.0f;
    aimAmount = Approach(aimAmount, aimTarget, dt * 8.0f);

    UpdateShotEffects(dt);

    const bool wantsToFire = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    const bool canShoot = wantsToFire && !isReloading && fireCooldown <= 0.0f && magazineAmmo > 0;

    if (wantsToFire && magazineAmmo <= 0 && reserveAmmo > 0) {
        StartReload();
    }

    if (canShoot) {
        fireCooldown = 0.095f;
        magazineAmmo--;
    }

    const Vector3 hipOffset = { 0.82f, 0.44f, -0.28f };
    const Vector3 aimOffset = { 0.67f, 0.02f, -0.055f };
    const Vector3 offset = Lerp3(hipOffset, aimOffset, aimAmount);

    Vector3 weaponPos = camera.position;
    weaponPos = Vector3Add(weaponPos, Vector3Scale(forward, offset.x));
    weaponPos = Vector3Add(weaponPos, Vector3Scale(right, offset.y));
    weaponPos = Vector3Add(weaponPos, Vector3Scale(up, offset.z));

    weaponPos = Vector3Add(weaponPos, Vector3Scale(forward, -0.08f * recoil));
    weaponPos = Vector3Add(weaponPos, Vector3Scale(up, 0.035f * recoil));

    if (isReloading) {
        const float reloadProgress = 1.0f - reloadTimer / AK_RELOAD_TIME;
        const float hideAmount = sinf(reloadProgress * PI);

        weaponPos = Vector3Add(weaponPos, Vector3Scale(up, -0.78f * hideAmount));
        weaponPos = Vector3Add(weaponPos, Vector3Scale(right, 0.18f * hideAmount));
        weaponPos = Vector3Add(weaponPos, Vector3Scale(forward, -0.16f * hideAmount));
    }

    const float scale = Lerp(1.35f, 1.12f, aimAmount);
    const Vector3 muzzlePos = Vector3Add(weaponPos, Vector3Scale(forward, 1.18f * scale));

    if (canShoot) {
        SpawnShotEffects(muzzlePos, forward, right, up);
        shotThisFrame = true;
    }

    Model weapon = ak;
    ApplyAkMaterialColors(&weapon);

    const float roll = Lerp(-0.05f, 0.0f, aimAmount);
    weapon.transform = MakeWeaponTransform(forward, right, up, roll);

    DrawModel(weapon, weaponPos, scale, WHITE);
    DrawShotEffects(camera, muzzlePos, forward);
}

bool AkWasShotThisFrame(void) {
    return shotThisFrame;
}

int AkGetMagazineAmmo(void) {
    return magazineAmmo;
}

int AkGetReserveAmmo(void) {
    return reserveAmmo;
}

bool AkIsReloading(void) {
    return reloadTimer > 0.0f;
}

void AkAddReserveAmmo(int amount) {
    reserveAmmo += amount;
}

void AkReset(void) {
    for (int i = 0; i < SMOKE_PARTICLE_COUNT; i++) {
        smokeParticles[i] = (SmokeParticle){ 0 };
    }

    for (int i = 0; i < TRACER_COUNT; i++) {
        tracers[i] = (BulletTracer){ 0 };
    }

    nextSmokeParticle = 0;
    nextTracer = 0;
    aimAmount = 0.0f;
    fireCooldown = 0.0f;
    flashTimer = 0.0f;
    recoil = 0.0f;
    reloadTimer = 0.0f;
    shotThisFrame = false;
    magazineAmmo = AK_MAGAZINE_SIZE;
    reserveAmmo = 90;
}
