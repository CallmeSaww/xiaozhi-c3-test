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

    void Init();
    void SetState(DasaimochiState state);
    DasaimochiState GetState() const { return current_state_; }

    // Retrieve the next frame descriptor (to be called inside LVGL lock/timer)
    const lv_img_dsc_t* GetNextFrame();

    static DasaimochiState MapEmotionToState(const char* emotion);
    static DasaimochiState MapStatusToState(const char* status);

private:
    void GenerateFrames();
    void ClearFrames();

    DasaimochiState current_state_ = DASAIMOCHI_IDLE;
    int current_frame_ = 0;
    int blink_counter_ = 0;
    int blink_interval_ = 25; // Blink every ~2.5 seconds (25 x 100ms)
    bool initialized_ = false;

    static constexpr int WIDTH = 32;
    static constexpr int HEIGHT = 32;
    static constexpr int FRAMES_PER_STATE = 4;

    uint8_t* frame_buffers_[DASAIMOCHI_STATE_COUNT][FRAMES_PER_STATE] = {};
    lv_img_dsc_t frame_dscs_[DASAIMOCHI_STATE_COUNT][FRAMES_PER_STATE] = {};
};

#endif // DASAIMOCHI_ANIM_H
