//
// Created by Runner on 26.06.2026.
//

#ifndef SHMUP_MODELS_H
#define SHMUP_MODELS_H

#include "raylib.h"

void drawAk(Model ak, Camera3D camera);
bool AkWasShotThisFrame(void);
int AkGetMagazineAmmo(void);
int AkGetReserveAmmo(void);
bool AkIsReloading(void);

#endif //SHMUP_MODELS_H
