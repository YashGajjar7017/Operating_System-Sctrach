/**
 * @file anim.c
 * @brief Fixed-Point Cubic-Bezier Ease-Out Animation Engine for Freestanding Kernel
 */

#include "anim.h"

#define FP_SCALE 1024

void anim_init(CubicAnimation *anim, int32_t start, int32_t target, uint32_t duration_frames) {
    if (!anim) return;
    anim->start_val = start;
    anim->target_val = target;
    anim->current_val = start;
    anim->duration_frames = duration_frames ? duration_frames : 1;
    anim->frame_index = 0;
    anim->is_complete = 0;
}

void anim_step(CubicAnimation *anim) {
    if (!anim || anim->is_complete) return;

    anim->frame_index++;
    if (anim->frame_index >= anim->duration_frames) {
        anim->current_val = anim->target_val;
        anim->is_complete = 1;
        return;
    }

    /* Fixed-point cubic ease-out: f(t) = 1 - (1 - t)^3 */
    int64_t t = ((int64_t)anim->frame_index * FP_SCALE) / anim->duration_frames;
    int64_t inv = FP_SCALE - t;
    int64_t inv3 = (inv * inv * inv) / (FP_SCALE * FP_SCALE);
    int64_t ease = FP_SCALE - inv3;

    int64_t delta = (int64_t)anim->target_val - anim->start_val;
    anim->current_val = anim->start_val + (int32_t)((delta * ease) / FP_SCALE);
}

int32_t anim_get_val(const CubicAnimation *anim) {
    if (!anim) return 0;
    return anim->current_val;
}
