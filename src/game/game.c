#include "raylib.h"
#include "raymath.h"

#include "game.h"
#include "player.h"
#include "embedded_assets.h"
#include "models.h"
#include "shoot/world.h"

#include <stdio.h>

#define TARGET_FPS 120

static void DrawAmmoHud(void) {
    const int marginRight = 34;
    const int marginBottom = 30;
    const int magazineFontSize = 48;
    const int reserveFontSize = 26;

    char magazineText[8];
    char reserveText[8];

    snprintf(magazineText, sizeof(magazineText), "%d", AkGetMagazineAmmo());
    snprintf(reserveText, sizeof(reserveText), "%d", AkGetReserveAmmo());
    const int magazineWidth = MeasureText(magazineText, magazineFontSize);
    const int slashWidth = MeasureText("/", reserveFontSize);
    const int reserveWidth = MeasureText(reserveText, reserveFontSize);
    const int totalWidth = magazineWidth + 12 + slashWidth + 8 + reserveWidth;
    const int baseX = GetScreenWidth() - marginRight - totalWidth;
    const int baseY = GetScreenHeight() - marginBottom - magazineFontSize;
    const int reserveY = baseY + magazineFontSize - reserveFontSize - 4;
    const Color reserveColor = AkIsReloading() ? (Color){ 210, 190, 120, 255 } : GRAY;

    DrawText(magazineText, baseX, baseY, magazineFontSize, WHITE);
    DrawText("/", baseX + magazineWidth + 12, reserveY, reserveFontSize, reserveColor);
    DrawText(reserveText, baseX + magazineWidth + 12 + slashWidth + 8, reserveY, reserveFontSize, reserveColor);
}

static void DrawHealthHud(const Player *player) {
    const int x = 28;
    const int y = GetScreenHeight() - 42;
    const int width = 260;
    const int height = 18;
    const float healthPercent = (float)player->health / (float)player->maxHealth;
    const int filledWidth = (int)((float)width * healthPercent);
    const Color healthColor = (healthPercent < 0.2f) ? RED : GREEN;

    DrawRectangle(x, y, width, height, DARKGRAY);
    DrawRectangle(x, y, filledWidth, height, healthColor);
    DrawRectangleLines(x, y, width, height, BLACK);
}

static void FormatSurvivalTime(float time, char *buffer, int bufferSize) {
    const int totalMilliseconds = (int)(time * 1000.0f);
    const int milliseconds = totalMilliseconds % 1000;
    const int totalSeconds = totalMilliseconds / 1000;
    const int seconds = totalSeconds % 60;
    const int minutes = totalSeconds / 60;

    snprintf(buffer, bufferSize, "%02d:%02d.%03d", minutes, seconds, milliseconds);
}

static void DrawTimerHud(float survivalTime, int wave) {
    char timeText[32];
    char hudText[64];
    const int fontSize = 28;
    const int margin = 28;

    FormatSurvivalTime(survivalTime, timeText, sizeof(timeText));
    snprintf(hudText, sizeof(hudText), "Wave %d  %s", wave, timeText);

    const int width = MeasureText(hudText, fontSize);
    DrawText(hudText, GetScreenWidth() - margin - width, margin, fontSize, DARKGRAY);
}

static void DrawGameOver(float survivalTime) {
    char timeText[32];
    char finalText[64];
    const char *title = "GAME OVER";
    const int titleSize = 72;
    const int finalSize = 28;

    FormatSurvivalTime(survivalTime, timeText, sizeof(timeText));
    snprintf(finalText, sizeof(finalText), "Survived: %s", timeText);

    const int titleWidth = MeasureText(title, titleSize);
    const int finalWidth = MeasureText(finalText, finalSize);
    const int centerX = GetScreenWidth() / 2;
    const int centerY = GetScreenHeight() / 2;

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.45f));
    DrawText(title, centerX - titleWidth / 2, centerY - 70, titleSize, RED);
    DrawText(finalText, centerX - finalWidth / 2, centerY + 18, finalSize, WHITE);
    DrawText("Press ENTER to restart", centerX - MeasureText("Press ENTER to restart", 24) / 2, centerY + 58, 24, LIGHTGRAY);
}

static Camera3D CreateStartCamera(void) {
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0, 2, 4 };
    camera.target = (Vector3){ 0, 2, 3 };
    camera.up = (Vector3){ 0, 1, 0 };
    camera.fovy = 60;
    camera.projection = CAMERA_PERSPECTIVE;

    return camera;
}

void GameRun(void) {
    InitWindow(1280, 720, "Rynner FPS");

    Camera3D camera = CreateStartCamera();

    DisableCursor();
    SetTargetFPS(TARGET_FPS);

    const Vector3 goal = { 0.0f, 1.0f, -8.0f };
    bool isWin = false;

    Player player = Player_Create((Vector3){ 0.0f, 0.0f, 4.0f });
    World world = World_Create();
    float survivalTime = 0.0f;
    bool gameOver = false;

    FILE *file = fopen("ak_embedded.glb", "wb");

    if (file != NULL) {
        fwrite(AK_MODEL_DATA, 1, AK_MODEL_SIZE, file);
        fclose(file);
    }

    Model ak = LoadModel("ak_embedded.glb");
    for (int i = 0; i < ak.materialCount; i++) {
        ak.materials[i].maps[MATERIAL_MAP_DIFFUSE].color = DARKGRAY;
    }

    while (!WindowShouldClose()) {
        const float dt = GetFrameTime();

        if (gameOver && IsKeyPressed(KEY_ENTER)) {
            player = Player_Create((Vector3){ 0.0f, 0.0f, 4.0f });
            world = World_Create();
            camera = CreateStartCamera();
            survivalTime = 0.0f;
            gameOver = false;
            AkReset();
        }

        if (!gameOver) {
            // UpdateCamera(&camera, CAMERA_FIRST_PERSON);
            // updateCamera(&camera, &player);
            Player_Update(&player, &camera, &world);
            World_Update(&world, dt, player.position);
            survivalTime += dt;

            const int damage = World_GetEnemyDamage(&world, player.position, player.radius);
            if (damage > 0) {
                Player_TakeDamage(&player, damage);
            }

            const int collectedAmmo = World_CollectAmmo(&world, player.position, player.radius);
            if (collectedAmmo > 0) {
                AkAddReserveAmmo(collectedAmmo);
            }

            if (player.health <= 0) {
                gameOver = true;
            }
        }

        /*
        Vector3 playerFlat = { player.position.x, 0.0f, player.position.z };
        Vector3 goalFlat = { goal.x, 0.0f, goal.z };

        float distanceToGoal = Vector3Distance(playerFlat, goalFlat);

        if (distanceToGoal < 2.0f) {
            isWin = true;
        }
        */

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
        World_Draw(&world);

        if (!gameOver) {
            drawAk(ak, camera);

            if (AkWasShotThisFrame()) {
                World_Shoot(&world, camera);
            }
        }

        World_DrawBulletMarks(&world);

        EndMode3D();
        DrawCircle(GetScreenWidth()/2, GetScreenHeight()/2, 3, LIGHTGRAY);
        /*
        if (isWin) {
            DrawText("YOU WIN!", 520, 320, 60, GREEN);
        }
        */

        DrawFPS(20, 20);
        DrawText("WASD + mouse. ESC to quit.", 20, 50, 20, DARKGRAY);
        DrawTimerHud(survivalTime, World_GetCurrentWave(&world));
        DrawHealthHud(&player);
        DrawAmmoHud();

        if (gameOver) {
            DrawGameOver(survivalTime);
        }

        EndDrawing();
    }

    UnloadModel(ak);
    remove("ak_embedded.glb");
    CloseWindow();
}
