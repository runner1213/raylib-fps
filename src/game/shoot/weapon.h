//
// Created by Runner on 26.06.2026.
//

#ifndef SHMUP_WEAPON_H
#define SHMUP_WEAPON_H

#include "raylib.h"

typedef struct BulletMark {
    Vector3 position;
    Vector3 normal;
    float size;
    float life;
} BulletMark;

#endif //SHMUP_WEAPON_H
