#ifndef DASAIMOCHI_ANIM_H
#define DASAIMOCHI_ANIM_H

#include <lvgl.h>
#include <cstdint>

enum DasaimochiState {
    DASAIMOCHI_IDLE = 0,
    DASAIMOCHI_LISTENING,
    DASAIMOCHI_SPEAKING,
    DASAIMOCHI_THINKING,
    DASAIMOCHI_HAPPY,
    DASAIMOCHI_SLEEPING,
    DASAIMOCHI_STATE_COUNT
};

class DasaimochiAnim {
public:
    DasaimochiAnim();
    ~DasaimochiAnim();

    void Init(int width = 128, int height = 64);
    void SetState(DasaimochiState state);
    DasaimochiState GetState() const { return current_state_; }

    // Retrieve the next frame descriptor (called inside LVGL lock/timer)
    const lv_img_dsc_t* GetNextFrame();

    static DasaimochiState MapEmotionToState(const char* emotion);
    static DasaimochiState MapStatusToState(const char* status);

private:
    void RenderFrame();

    DasaimochiState current_state_ = DASAIMOCHI_IDLE;
    int tick_count_ = 0;
    int blink_timer_ = 0;
    int blink_stage_ = 0; // 0=open, 1=closing, 2=closed, 3=opening
    bool initialized_ = false;

    int width_ = 128;
    int height_ = 64;

    uint8_t* frame_buffer_ = nullptr;
    lv_img_dsc_t frame_dsc_ = {};
};

#endif // DASAIMOCHI_ANIM_H
