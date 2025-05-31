#ifndef ANIM_H
#define ANIM_H

#include <stdint.h>
#include <stdbool.h>

struct animation {
    const uint8_t (*frames)[8][128] ; // Pointer to an array of frames, each frame is 8x128 pixels
    const uint8_t *frame_counts;
    uint32_t start_idx, end_idx, init_idx;
    int8_t frame_step;
    bool loop;
};

#endif // ANIM_H