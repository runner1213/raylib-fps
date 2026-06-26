#include "raylib.h"
#include "raymath.h"

#include "game.h"
#include "player.h"
#include "embedded_assets.h"
#include "models.h"
#include "shoot/world.h"

#include <stdio.h>

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

void GameRun(void) {
    InitWindow(1280, 720, "Rynner FPS");

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0, 2, 4 };
    camera.target = (Vector3){ 0, 2, 3 };
    camera.up = (Vector3){ 0, 1, 0 };
    camera.fovy = 60;
    camera.projection = CAMERA_PERSPECTIVE;

    DisableCursor();
    SetTargetFPS(60);

    const Vector3 goal = { 0.0f, 1.0f, -8.0f };
    bool isWin = false;

    Player player = Player_Create((Vector3){ 0.0f, 0.0f, 4.0f });
    World world = World_Create();

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
        // UpdateCamera(&camera, CAMERA_FIRST_PERSON);
        // updateCamera(&camera, &player);
        Player_Update(&player, &camera, &world);
        World_Update(&world, GetFrameTime());

        Vector3 playerFlat = { player.position.x, 0.0f, player.position.z };
        Vector3 goalFlat = { goal.x, 0.0f, goal.z };

        float distanceToGoal = Vector3Distance(playerFlat, goalFlat);

        if (distanceToGoal < 2.0f) {
            isWin = true;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
        World_Draw(&world);
        drawAk(ak, camera);

        if (AkWasShotThisFrame()) {
            World_Shoot(&world, camera);
        }

        World_DrawBulletMarks(&world);

        EndMode3D();
        DrawCircle(GetScreenWidth()/2, GetScreenHeight()/2, 3, LIGHTGRAY);
        if (isWin) {
            DrawText("YOU WIN!", 520, 320, 60, GREEN);
        }

        DrawFPS(20, 20);
        DrawText("WASD + mouse. ESC to quit.", 20, 50, 20, DARKGRAY);
        DrawAmmoHud();

        EndDrawing();
    }

    UnloadModel(ak);
    remove("ak_embedded.glb");
    CloseWindow();
}
