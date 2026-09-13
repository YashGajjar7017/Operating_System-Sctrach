/**
 * @file anim.h
 * @brief 60 FPS Cubic-Bezier Hardware Entrance & Window Animation Engine
 */

#ifndef _KERNEL_GUI_ANIM_H_
#define _KERNEL_GUI_ANIM_H_

#include <stdint.h>
#include <stddef.h>

typedef struct {
    int32_t  start_val;
    int32_t  target_val;
    int32_t  current_val;
    uint32_t duration_frames;
    uint32_t frame_index;
    uint8_t  is_complete;
} CubicAnimation;

void anim_init(CubicAnimation *anim, int32_t start, int32_t target, uint32_t duration_frames);
void anim_step(CubicAnimation *anim);
int32_t anim_get_val(const CubicAnimation *anim);

#endif /* _KERNEL_GUI_ANIM_H_ */
