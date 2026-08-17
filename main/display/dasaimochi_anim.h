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

    const lv_img_dsc_t* GetNextFrame();

    static DasaimochiState MapEmotionToState(const char* emotion);
    static DasaimochiState MapStatusToState(const char* status);

private:
    void RenderFrame();
    void RenderIdleFrame();

    DasaimochiState current_state_ = DASAIMOCHI_IDLE;
    int tick_count_ = 0;
    int idle_frame_idx_ = 0;
    bool initialized_ = false;

    int width_ = 128;
    int height_ = 64;

    uint8_t* frame_buffer_ = nullptr;
    lv_img_dsc_t frame_dsc_ = {};
};

#endif // DASAIMOCHI_ANIM_H
