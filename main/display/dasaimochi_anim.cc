#include "dasaimochi_anim.h"
#include "dasaimochi_frames.h"
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <string>

#define TAG "DasaimochiAnim"

namespace {

inline void SetPixel(uint8_t* buf, int w, int h, int x, int y, uint8_t val) {
    if (x >= 0 && x < w && y >= 0 && y < h) {
        buf[y * w + x] = val;
    }
}

void DrawFilledRect(uint8_t* buf, int w, int h, int x, int y, int rw, int rh, uint8_t val) {
    for (int ry = y; ry < y + rh; ry++) {
        for (int rx = x; rx < x + rw; rx++) {
            SetPixel(buf, w, h, rx, ry, val);
        }
    }
}

void DrawFilledCircle(uint8_t* buf, int w, int h, int cx, int cy, int r, uint8_t val) {
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x * x + y * y <= r * r) {
                SetPixel(buf, w, h, cx + x, cy + y, val);
            }
        }
    }
}

void DrawDasaiPill(uint8_t* buf, int w, int h, int cx, int cy, int pw, int ph, uint8_t val) {
    if (pw <= 0 || ph <= 0) return;
    int x0 = cx - pw / 2;
    int y0 = cy - ph / 2;
    if (ph <= 4) {
        DrawFilledRect(buf, w, h, x0, y0, pw, ph, val);
        return;
    }
    int r = std::min(pw / 2, 6);
    DrawFilledRect(buf, w, h, x0, y0 + r, pw, ph - 2 * r, val);
    for (int dy = 0; dy < r; dy++) {
        int inset = r - (int)std::sqrt(std::max(0, r * r - (r - dy) * (r - dy)));
        DrawFilledRect(buf, w, h, x0 + inset, y0 + dy, pw - 2 * inset, 1, val);
        DrawFilledRect(buf, w, h, x0 + inset, y0 + ph - 1 - dy, pw - 2 * inset, 1, val);
    }
}

void DrawDasaiEye(uint8_t* buf, int w, int h, int cx, int cy, int ew, int eh, int pupil_shift_x = 0, int pupil_shift_y = 0) {
    DrawDasaiPill(buf, w, h, cx, cy, ew, eh, 0xFF);
    if (eh >= 14 && ew >= 10) {
        int hx = cx + ew / 4 + pupil_shift_x;
        int hy = cy - eh / 4 + pupil_shift_y;
        SetPixel(buf, w, h, hx, hy, 0x00);
        SetPixel(buf, w, h, hx + 1, hy, 0x00);
        SetPixel(buf, w, h, hx, hy + 1, 0x00);
        SetPixel(buf, w, h, hx + 1, hy + 1, 0x00);
    }
}

void DrawHappyEyeArc(uint8_t* buf, int w, int h, int cx, int cy, int radius) {
    for (int dx = -radius; dx <= radius; dx++) {
        int dy = (int)std::sqrt(std::max(0, radius * radius - dx * dx));
        SetPixel(buf, w, h, cx + dx, cy - dy, 0xFF);
        SetPixel(buf, w, h, cx + dx, cy - dy + 1, 0xFF);
        SetPixel(buf, w, h, cx + dx, cy - dy + 2, 0xFF);
    }
}

void DrawSleepyEyeArc(uint8_t* buf, int w, int h, int cx, int cy, int radius) {
    for (int dx = -radius; dx <= radius; dx++) {
        int dy = (int)std::sqrt(std::max(0, radius * radius - dx * dx));
        SetPixel(buf, w, h, cx + dx, cy + dy - 2, 0xFF);
        SetPixel(buf, w, h, cx + dx, cy + dy - 1, 0xFF);
        SetPixel(buf, w, h, cx + dx, cy + dy, 0xFF);
    }
}

void DrawBlush(uint8_t* buf, int w, int h, int cx, int cy, bool left_side) {
    int dir = left_side ? 1 : -1;
    for (int i = -1; i <= 1; i++) {
        int sx = cx + i * 4;
        int sy = cy;
        for (int step = -2; step <= 2; step++) {
            SetPixel(buf, w, h, sx + step * dir, sy + step, 0xFF);
            SetPixel(buf, w, h, sx + step * dir + 1, sy + step, 0xFF);
        }
    }
}

void DrawSmileMouth(uint8_t* buf, int w, int h, int cx, int cy, int radius) {
    for (int dx = -radius; dx <= radius; dx++) {
        int dy = (int)std::sqrt(std::max(0, radius * radius - dx * dx));
        SetPixel(buf, w, h, cx + dx, cy + dy - 1, 0xFF);
        SetPixel(buf, w, h, cx + dx, cy + dy, 0xFF);
    }
}

void DrawOpenMouth(uint8_t* buf, int w, int h, int cx, int cy, int mw, int mh) {
    int rx = mw / 2;
    int ry = mh / 2;
    for (int y = -ry; y <= ry; y++) {
        for (int x = -rx; x <= rx; x++) {
            if ((rx > 0 && ry > 0) && (x * x * ry * ry + y * y * rx * rx) <= (rx * rx * ry * ry)) {
                SetPixel(buf, w, h, cx + x, cy + y, 0xFF);
            }
        }
    }
}

void DrawCatMouth(uint8_t* buf, int w, int h, int cx, int cy) {
    SetPixel(buf, w, h, cx - 4, cy - 1, 0xFF);
    SetPixel(buf, w, h, cx - 3, cy, 0xFF);
    SetPixel(buf, w, h, cx - 2, cy, 0xFF);
    SetPixel(buf, w, h, cx - 1, cy - 1, 0xFF);
    SetPixel(buf, w, h, cx, cy - 1, 0xFF);
    SetPixel(buf, w, h, cx + 1, cy, 0xFF);
    SetPixel(buf, w, h, cx + 2, cy, 0xFF);
    SetPixel(buf, w, h, cx + 3, cy - 1, 0xFF);
}

void DrawZ(uint8_t* buf, int w, int h, int x, int y, int size) {
    DrawFilledRect(buf, w, h, x, y, size, 1, 0xFF);
    for (int i = 0; i < size; i++) {
        SetPixel(buf, w, h, x + size - 1 - i, y + i, 0xFF);
    }
    DrawFilledRect(buf, w, h, x, y + size - 1, size, 1, 0xFF);
}

void DrawThinkingDots(uint8_t* buf, int w, int h, int x, int y, int frame) {
    for (int i = 0; i < 3; i++) {
        int r = (frame == i) ? 3 : 2;
        DrawFilledCircle(buf, w, h, x + i * 8, y - (frame == i ? 2 : 0), r, 0xFF);
    }
}

void DrawSparkle(uint8_t* buf, int w, int h, int cx, int cy) {
    SetPixel(buf, w, h, cx, cy - 3, 0xFF);
    SetPixel(buf, w, h, cx, cy + 3, 0xFF);
    SetPixel(buf, w, h, cx - 3, cy, 0xFF);
    SetPixel(buf, w, h, cx + 3, cy, 0xFF);
    DrawFilledRect(buf, w, h, cx - 1, cy - 1, 3, 3, 0xFF);
}

} // namespace

DasaimochiAnim::DasaimochiAnim() {}

DasaimochiAnim::~DasaimochiAnim() {
    if (frame_buffer_ != nullptr) {
        free(frame_buffer_);
        frame_buffer_ = nullptr;
    }
}

void DasaimochiAnim::Init(int width, int height) {
    if (initialized_ && frame_buffer_ != nullptr && width_ == width && height_ == height) {
        return;
    }
    width_ = width;
    height_ = height;

    if (frame_buffer_ != nullptr) {
        free(frame_buffer_);
    }

    size_t buf_size = width_ * height_;
    frame_buffer_ = (uint8_t*)heap_caps_malloc(buf_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!frame_buffer_) {
        frame_buffer_ = (uint8_t*)malloc(buf_size);
    }
    if (frame_buffer_) {
        memset(frame_buffer_, 0, buf_size);
    }

    memset(&frame_dsc_, 0, sizeof(lv_img_dsc_t));
    frame_dsc_.header.magic = LV_IMAGE_HEADER_MAGIC;
    frame_dsc_.header.cf = LV_COLOR_FORMAT_A8;
    frame_dsc_.header.w = width_;
    frame_dsc_.header.h = height_;
    frame_dsc_.header.stride = width_;
    frame_dsc_.data_size = buf_size;
    frame_dsc_.data = frame_buffer_;

    initialized_ = true;
    ESP_LOGI(TAG, "DasaimochiAnim initialized with %d idle frames, resolution %dx%d", DASAIMOCHI_TOTAL_IDLE_FRAMES, width_, height_);
}

void DasaimochiAnim::SetState(DasaimochiState state) {
    if (state != current_state_) {
        current_state_ = state;
        tick_count_ = 0;
    }
}

void DasaimochiAnim::RenderIdleFrame() {
    if (!frame_buffer_) return;

    // Unpack 1-bit monochrome frame (1024 bytes) into 8-bit LVGL buffer
    const uint8_t* raw_frame = g_dasaimochi_frames[idle_frame_idx_];
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 128; x++) {
            int byte_idx = y * 16 + (x / 8);
            int bit_idx = 7 - (x % 8);
            uint8_t bit = (raw_frame[byte_idx] >> bit_idx) & 0x01;
            uint8_t val = bit ? 0xFF : 0x00;
            if (invert_colors_) {
                val = 0xFF - val;
            }
            frame_buffer_[y * width_ + x] = val;
        }
    }

    idle_frame_idx_ = (idle_frame_idx_ + 1) % DASAIMOCHI_TOTAL_IDLE_FRAMES;
}

void DasaimochiAnim::RenderFrame() {
    if (!frame_buffer_) return;

    if (current_state_ == DASAIMOCHI_IDLE) {
        RenderIdleFrame();
        return;
    }

    memset(frame_buffer_, 0, width_ * height_);

    int eye_dist = 50;
    int eye_left_x = 64 - eye_dist / 2;  // 39
    int eye_right_x = 64 + eye_dist / 2; // 89
    int eye_y = 32;
    int eye_w = 16;
    int eye_h = 28;

    int blush_left_x = eye_left_x - eye_w / 2 - 6;
    int blush_right_x = eye_right_x + eye_w / 2 + 6;
    int blush_y = 48;

    int mouth_x = 64;
    int mouth_y = 48;

    tick_count_++;

    switch (current_state_) {
        case DASAIMOCHI_LISTENING: {
            DrawDasaiEye(frame_buffer_, width_, height_, eye_left_x, eye_y, eye_w + 2, eye_h + 2);
            DrawDasaiEye(frame_buffer_, width_, height_, eye_right_x, eye_y, eye_w + 2, eye_h + 2);

            if ((tick_count_ / 2) % 2 == 0) {
                DrawSparkle(frame_buffer_, width_, height_, eye_left_x - 16, eye_y - 12);
                DrawSparkle(frame_buffer_, width_, height_, eye_right_x + 16, eye_y - 12);
            }

            DrawOpenMouth(frame_buffer_, width_, height_, mouth_x, mouth_y, 6, 6);
            DrawBlush(frame_buffer_, width_, height_, blush_left_x, blush_y, true);
            DrawBlush(frame_buffer_, width_, height_, blush_right_x, blush_y, false);
            break;
        }

        case DASAIMOCHI_SPEAKING: {
            int bounce = ((tick_count_ / 2) % 2 == 0) ? 1 : 0;
            DrawDasaiEye(frame_buffer_, width_, height_, eye_left_x, eye_y - bounce, eye_w, eye_h - 2);
            DrawDasaiEye(frame_buffer_, width_, height_, eye_right_x, eye_y - bounce, eye_w, eye_h - 2);

            int mouth_phase = (tick_count_ / 2) % 4;
            if (mouth_phase == 0) DrawOpenMouth(frame_buffer_, width_, height_, mouth_x, mouth_y, 8, 5);
            else if (mouth_phase == 1) DrawOpenMouth(frame_buffer_, width_, height_, mouth_x, mouth_y, 12, 8);
            else if (mouth_phase == 2) DrawOpenMouth(frame_buffer_, width_, height_, mouth_x, mouth_y, 14, 11);
            else DrawSmileMouth(frame_buffer_, width_, height_, mouth_x, mouth_y, 5);

            DrawBlush(frame_buffer_, width_, height_, blush_left_x, blush_y - bounce, true);
            DrawBlush(frame_buffer_, width_, height_, blush_right_x, blush_y - bounce, false);
            break;
        }

        case DASAIMOCHI_THINKING: {
            DrawDasaiEye(frame_buffer_, width_, height_, eye_left_x + 4, eye_y - 4, eye_w, eye_h - 4, 2, -2);
            DrawDasaiEye(frame_buffer_, width_, height_, eye_right_x + 4, eye_y - 4, eye_w, eye_h - 4, 2, -2);

            int dot_frame = (tick_count_ / 3) % 3;
            DrawThinkingDots(frame_buffer_, width_, height_, 104, 12, dot_frame);
            DrawSmileMouth(frame_buffer_, width_, height_, mouth_x + 2, mouth_y, 3);

            DrawBlush(frame_buffer_, width_, height_, blush_left_x, blush_y, true);
            DrawBlush(frame_buffer_, width_, height_, blush_right_x, blush_y, false);
            break;
        }

        case DASAIMOCHI_HAPPY: {
            DrawHappyEyeArc(frame_buffer_, width_, height_, eye_left_x, eye_y + 4, 9);
            DrawHappyEyeArc(frame_buffer_, width_, height_, eye_right_x, eye_y + 4, 9);

            DrawSmileMouth(frame_buffer_, width_, height_, mouth_x, mouth_y - 2, 7);
            DrawBlush(frame_buffer_, width_, height_, blush_left_x, blush_y, true);
            DrawBlush(frame_buffer_, width_, height_, blush_right_x, blush_y, false);
            break;
        }

        case DASAIMOCHI_SLEEPING: {
            DrawSleepyEyeArc(frame_buffer_, width_, height_, eye_left_x, eye_y + 4, 7);
            DrawSleepyEyeArc(frame_buffer_, width_, height_, eye_right_x, eye_y + 4, 7);

            int z_anim = (tick_count_ / 3) % 4;
            DrawZ(frame_buffer_, width_, height_, 100, 18 - z_anim, 4);
            DrawZ(frame_buffer_, width_, height_, 108, 12 - z_anim, 6);
            DrawZ(frame_buffer_, width_, height_, 116, 6 - z_anim, 8);

            DrawCatMouth(frame_buffer_, width_, height_, mouth_x, mouth_y);
            break;
        }

        default:
            break;
    }

    if (invert_colors_) {
        int total = width_ * height_;
        for (int i = 0; i < total; i++) {
            frame_buffer_[i] = 0xFF - frame_buffer_[i];
        }
    }
}

const lv_img_dsc_t* DasaimochiAnim::GetNextFrame() {
    if (!initialized_ || !frame_buffer_) {
        return nullptr;
    }
    RenderFrame();
    return &frame_dsc_;
}

DasaimochiState DasaimochiAnim::MapEmotionToState(const char* emotion) {
    if (!emotion || emotion[0] == '\0') return DASAIMOCHI_IDLE;
    std::string em = emotion;
    if (em == "neutral" || em == "robot_1" || em == "robot_2") return DASAIMOCHI_IDLE;
    if (em == "listening") return DASAIMOCHI_LISTENING;
    if (em == "speaking" || em == "happy") return DASAIMOCHI_SPEAKING;
    if (em == "thinking") return DASAIMOCHI_THINKING;
    if (em == "excited" || em == "loving" || em == "joyful") return DASAIMOCHI_HAPPY;
    if (em == "sleeping" || em == "sleepy") return DASAIMOCHI_SLEEPING;
    return DASAIMOCHI_IDLE;
}

DasaimochiState DasaimochiAnim::MapStatusToState(const char* status) {
    if (!status) return DASAIMOCHI_IDLE;
    std::string st = status;
    if (st.find("听") != std::string::npos || st.find("Nghe") != std::string::npos || st.find("Listen") != std::string::npos) {
        return DASAIMOCHI_LISTENING;
    }
    if (st.find("说") != std::string::npos || st.find("Nói") != std::string::npos || st.find("Speak") != std::string::npos) {
        return DASAIMOCHI_SPEAKING;
    }
    if (st.find("想") != std::string::npos || st.find("Nghĩ") != std::string::npos || st.find("Think") != std::string::npos) {
        return DASAIMOCHI_THINKING;
    }
    if (st.find("睡") != std::string::npos || st.find("Ngủ") != std::string::npos || st.find("Sleep") != std::string::npos) {
        return DASAIMOCHI_SLEEPING;
    }
    return DASAIMOCHI_IDLE;
}
