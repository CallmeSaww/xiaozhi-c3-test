#include "dasaimochi_anim.h"
#include <esp_log.h>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <string>

#define TAG "DasaimochiAnim"

namespace {

inline void SetPixel(uint8_t* buf, int x, int y, uint8_t val) {
    if (x >= 0 && x < 32 && y >= 0 && y < 32) {
        buf[y * 32 + x] = val;
    }
}

void DrawFilledRect(uint8_t* buf, int x, int y, int w, int h, uint8_t val) {
    for (int ry = y; ry < y + h; ry++) {
        for (int rx = x; rx < x + w; rx++) {
            SetPixel(buf, rx, ry, val);
        }
    }
}

void DrawPillEye(uint8_t* buf, int x, int y, int w, int h, bool pupil_highlight) {
    if (h <= 0 || w <= 0) return;
    if (h <= 3) {
        DrawFilledRect(buf, x, y, w, h, 0xFF);
        return;
    }
    // Vertical capsule (pill)
    DrawFilledRect(buf, x, y + 2, w, h - 4, 0xFF);
    DrawFilledRect(buf, x + 1, y + 1, w - 2, h - 2, 0xFF);
    DrawFilledRect(buf, x + 2, y, w - 4, h, 0xFF);

    // Pupil highlight in top-right
    if (pupil_highlight && w >= 6 && h >= 8) {
        SetPixel(buf, x + w - 3, y + 2, 0x00);
        SetPixel(buf, x + w - 2, y + 2, 0x00);
        SetPixel(buf, x + w - 3, y + 3, 0x00);
        SetPixel(buf, x + w - 2, y + 3, 0x00);
    }
}

void DrawArcEye(uint8_t* buf, int cx, int cy, int radius) {
    for (int dx = -radius; dx <= radius; dx++) {
        int dy = radius - std::abs(dx);
        SetPixel(buf, cx + dx, cy - dy, 0xFF);
        SetPixel(buf, cx + dx, cy - dy + 1, 0xFF);
    }
}

void DrawHeartEye(uint8_t* buf, int cx, int cy) {
    static const int heart[7][7] = {
        {0, 1, 1, 0, 1, 1, 0},
        {1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1},
        {0, 1, 1, 1, 1, 1, 0},
        {0, 0, 1, 1, 1, 0, 0},
        {0, 0, 0, 1, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0}
    };
    for (int r = 0; r < 7; r++) {
        for (int c = 0; c < 7; c++) {
            if (heart[r][c]) {
                SetPixel(buf, cx - 3 + c, cy - 3 + r, 0xFF);
            }
        }
    }
}

void DrawStarEye(uint8_t* buf, int cx, int cy) {
    static const int star[7][7] = {
        {0, 0, 0, 1, 0, 0, 0},
        {0, 1, 1, 1, 1, 1, 0},
        {0, 0, 1, 1, 1, 0, 0},
        {1, 1, 1, 1, 1, 1, 1},
        {0, 0, 1, 1, 1, 0, 0},
        {0, 1, 0, 0, 0, 1, 0},
        {1, 0, 0, 0, 0, 0, 1}
    };
    for (int r = 0; r < 7; r++) {
        for (int c = 0; c < 7; c++) {
            if (star[r][c]) {
                SetPixel(buf, cx - 3 + c, cy - 3 + r, 0xFF);
            }
        }
    }
}

void DrawMouth(uint8_t* buf, int cx, int cy, int rx, int ry) {
    for (int y = -ry; y <= ry; y++) {
        for (int x = -rx; x <= rx; x++) {
            if ((rx > 0 && ry > 0) && (x * x * ry * ry + y * y * rx * rx) <= (rx * rx * ry * ry)) {
                SetPixel(buf, cx + x, cy + y, 0xFF);
            }
        }
    }
}

void DrawZzz(uint8_t* buf, int x, int y, int size) {
    if (size == 1) { // Small 'z'
        DrawFilledRect(buf, x, y, 4, 1, 0xFF);
        SetPixel(buf, x + 2, y + 1, 0xFF);
        SetPixel(buf, x + 1, y + 2, 0xFF);
        DrawFilledRect(buf, x, y + 3, 4, 1, 0xFF);
    } else { // Large 'Z'
        DrawFilledRect(buf, x, y, 6, 1, 0xFF);
        SetPixel(buf, x + 4, y + 1, 0xFF);
        SetPixel(buf, x + 3, y + 2, 0xFF);
        SetPixel(buf, x + 2, y + 3, 0xFF);
        SetPixel(buf, x + 1, y + 4, 0xFF);
        DrawFilledRect(buf, x, y + 5, 6, 1, 0xFF);
    }
}

} // namespace

DasaimochiAnim::DasaimochiAnim() {}

DasaimochiAnim::~DasaimochiAnim() {
    ClearFrames();
}

void DasaimochiAnim::ClearFrames() {
    for (int s = 0; s < DASAIMOCHI_STATE_COUNT; s++) {
        for (int f = 0; f < FRAMES_PER_STATE; f++) {
            if (frame_buffers_[s][f] != nullptr) {
                delete[] frame_buffers_[s][f];
                frame_buffers_[s][f] = nullptr;
            }
        }
    }
    initialized_ = false;
}

void DasaimochiAnim::Init() {
    if (initialized_) return;
    GenerateFrames();
    initialized_ = true;
}

void DasaimochiAnim::GenerateFrames() {
    ClearFrames();

    for (int s = 0; s < DASAIMOCHI_STATE_COUNT; s++) {
        for (int f = 0; f < FRAMES_PER_STATE; f++) {
            uint8_t* buf = new uint8_t[WIDTH * HEIGHT]();
            frame_buffers_[s][f] = buf;

            memset(&frame_dscs_[s][f], 0, sizeof(lv_img_dsc_t));
            frame_dscs_[s][f].header.magic = LV_IMAGE_HEADER_MAGIC;
            frame_dscs_[s][f].header.cf = LV_COLOR_FORMAT_A8;
            frame_dscs_[s][f].header.w = WIDTH;
            frame_dscs_[s][f].header.h = HEIGHT;
            frame_dscs_[s][f].header.stride = WIDTH;
            frame_dscs_[s][f].data = buf;
            frame_dscs_[s][f].data_size = WIDTH * HEIGHT;
        }
    }

    // --- State 0: IDLE (Blinking Pill Eyes) ---
    // Frame 0: Open eyes
    DrawPillEye(frame_buffers_[DASAIMOCHI_IDLE][0], 5, 8, 7, 16, true);
    DrawPillEye(frame_buffers_[DASAIMOCHI_IDLE][0], 20, 8, 7, 16, true);

    // Frame 1: Half-closed
    DrawPillEye(frame_buffers_[DASAIMOCHI_IDLE][1], 5, 12, 7, 8, false);
    DrawPillEye(frame_buffers_[DASAIMOCHI_IDLE][1], 20, 12, 7, 8, false);

    // Frame 2: Closed
    DrawFilledRect(frame_buffers_[DASAIMOCHI_IDLE][2], 4, 15, 9, 2, 0xFF);
    DrawFilledRect(frame_buffers_[DASAIMOCHI_IDLE][2], 19, 15, 9, 2, 0xFF);

    // Frame 3: Opening
    DrawPillEye(frame_buffers_[DASAIMOCHI_IDLE][3], 5, 11, 7, 10, false);
    DrawPillEye(frame_buffers_[DASAIMOCHI_IDLE][3], 20, 11, 7, 10, false);

    // --- State 1: LISTENING (Curious Wide Eyes + Antenna Waves) ---
    // Frame 0: Wide open eyes
    DrawPillEye(frame_buffers_[DASAIMOCHI_LISTENING][0], 4, 7, 8, 18, true);
    DrawPillEye(frame_buffers_[DASAIMOCHI_LISTENING][0], 20, 7, 8, 18, true);
    // Waves: left (1, 12..20), right (30, 12..20)
    DrawFilledRect(frame_buffers_[DASAIMOCHI_LISTENING][0], 1, 12, 1, 8, 0xFF);
    DrawFilledRect(frame_buffers_[DASAIMOCHI_LISTENING][0], 30, 12, 1, 8, 0xFF);

    // Frame 1: Eyes shift slightly up
    DrawPillEye(frame_buffers_[DASAIMOCHI_LISTENING][1], 4, 5, 8, 18, true);
    DrawPillEye(frame_buffers_[DASAIMOCHI_LISTENING][1], 20, 5, 8, 18, true);
    DrawFilledRect(frame_buffers_[DASAIMOCHI_LISTENING][1], 0, 10, 2, 12, 0xFF);
    DrawFilledRect(frame_buffers_[DASAIMOCHI_LISTENING][1], 30, 10, 2, 12, 0xFF);

    // Frame 2: Happy arc eyes + antenna pulse
    DrawArcEye(frame_buffers_[DASAIMOCHI_LISTENING][2], 8, 16, 4);
    DrawArcEye(frame_buffers_[DASAIMOCHI_LISTENING][2], 24, 16, 4);
    DrawFilledRect(frame_buffers_[DASAIMOCHI_LISTENING][2], 1, 12, 1, 8, 0xFF);
    DrawFilledRect(frame_buffers_[DASAIMOCHI_LISTENING][2], 30, 12, 1, 8, 0xFF);

    // Frame 3: Wide eyes + Wink right eye
    DrawPillEye(frame_buffers_[DASAIMOCHI_LISTENING][3], 4, 7, 8, 18, true);
    DrawArcEye(frame_buffers_[DASAIMOCHI_LISTENING][3], 24, 16, 4);

    // --- State 2: SPEAKING (Talking Eyes & Mouth) ---
    // Frame 0: Happy arc eyes + small mouth
    DrawArcEye(frame_buffers_[DASAIMOCHI_SPEAKING][0], 8, 12, 4);
    DrawArcEye(frame_buffers_[DASAIMOCHI_SPEAKING][0], 24, 12, 4);
    DrawMouth(frame_buffers_[DASAIMOCHI_SPEAKING][0], 16, 23, 2, 2);

    // Frame 1: Happy arc eyes + open mouth
    DrawArcEye(frame_buffers_[DASAIMOCHI_SPEAKING][1], 8, 12, 4);
    DrawArcEye(frame_buffers_[DASAIMOCHI_SPEAKING][1], 24, 12, 4);
    DrawMouth(frame_buffers_[DASAIMOCHI_SPEAKING][1], 16, 23, 4, 4);

    // Frame 2: Pill eyes + open mouth
    DrawPillEye(frame_buffers_[DASAIMOCHI_SPEAKING][2], 5, 8, 7, 14, true);
    DrawPillEye(frame_buffers_[DASAIMOCHI_SPEAKING][2], 20, 8, 7, 14, true);
    DrawMouth(frame_buffers_[DASAIMOCHI_SPEAKING][2], 16, 24, 3, 4);

    // Frame 3: Happy arc eyes + medium mouth
    DrawArcEye(frame_buffers_[DASAIMOCHI_SPEAKING][3], 8, 12, 4);
    DrawArcEye(frame_buffers_[DASAIMOCHI_SPEAKING][3], 24, 12, 4);
    DrawMouth(frame_buffers_[DASAIMOCHI_SPEAKING][3], 16, 23, 3, 2);

    // --- State 3: THINKING (Eyes Scanning Left to Right) ---
    // Frame 0: Look Left
    DrawPillEye(frame_buffers_[DASAIMOCHI_THINKING][0], 2, 9, 7, 15, true);
    DrawPillEye(frame_buffers_[DASAIMOCHI_THINKING][0], 17, 9, 7, 15, true);

    // Frame 1: Look Top-Left
    DrawPillEye(frame_buffers_[DASAIMOCHI_THINKING][1], 2, 5, 7, 15, true);
    DrawPillEye(frame_buffers_[DASAIMOCHI_THINKING][1], 17, 5, 7, 15, true);

    // Frame 2: Look Right
    DrawPillEye(frame_buffers_[DASAIMOCHI_THINKING][2], 8, 9, 7, 15, true);
    DrawPillEye(frame_buffers_[DASAIMOCHI_THINKING][2], 23, 9, 7, 15, true);

    // Frame 3: Look Top-Right
    DrawPillEye(frame_buffers_[DASAIMOCHI_THINKING][3], 8, 5, 7, 15, true);
    DrawPillEye(frame_buffers_[DASAIMOCHI_THINKING][3], 23, 5, 7, 15, true);

    // --- State 4: HAPPY (Heart / Star / Bouncing Arc Eyes) ---
    // Frame 0: Heart eyes
    DrawHeartEye(frame_buffers_[DASAIMOCHI_HAPPY][0], 8, 14);
    DrawHeartEye(frame_buffers_[DASAIMOCHI_HAPPY][0], 24, 14);

    // Frame 1: High bouncing happy arcs
    DrawArcEye(frame_buffers_[DASAIMOCHI_HAPPY][1], 8, 10, 5);
    DrawArcEye(frame_buffers_[DASAIMOCHI_HAPPY][1], 24, 10, 5);

    // Frame 2: Star eyes
    DrawStarEye(frame_buffers_[DASAIMOCHI_HAPPY][2], 8, 14);
    DrawStarEye(frame_buffers_[DASAIMOCHI_HAPPY][2], 24, 14);

    // Frame 3: Standard happy arcs
    DrawArcEye(frame_buffers_[DASAIMOCHI_HAPPY][3], 8, 14, 4);
    DrawArcEye(frame_buffers_[DASAIMOCHI_HAPPY][3], 24, 14, 4);

    // --- State 5: SLEEPING (Closed Eyes + Drifting Zzz) ---
    // Frame 0: Closed 'u' eyes + small z
    DrawArcEye(frame_buffers_[DASAIMOCHI_SLEEPING][0], 8, 18, 3);
    DrawArcEye(frame_buffers_[DASAIMOCHI_SLEEPING][0], 24, 18, 3);
    DrawZzz(frame_buffers_[DASAIMOCHI_SLEEPING][0], 25, 4, 1);

    // Frame 1: Closed '-' eyes + large Z
    DrawFilledRect(frame_buffers_[DASAIMOCHI_SLEEPING][1], 5, 17, 7, 2, 0xFF);
    DrawFilledRect(frame_buffers_[DASAIMOCHI_SLEEPING][1], 20, 17, 7, 2, 0xFF);
    DrawZzz(frame_buffers_[DASAIMOCHI_SLEEPING][1], 22, 2, 2);

    // Frame 2: Closed 'u' eyes + small z higher
    DrawArcEye(frame_buffers_[DASAIMOCHI_SLEEPING][2], 8, 18, 3);
    DrawArcEye(frame_buffers_[DASAIMOCHI_SLEEPING][2], 24, 18, 3);
    DrawZzz(frame_buffers_[DASAIMOCHI_SLEEPING][2], 26, 2, 1);

    // Frame 3: Closed '-' eyes + large Z
    DrawFilledRect(frame_buffers_[DASAIMOCHI_SLEEPING][3], 5, 17, 7, 2, 0xFF);
    DrawFilledRect(frame_buffers_[DASAIMOCHI_SLEEPING][3], 20, 17, 7, 2, 0xFF);
    DrawZzz(frame_buffers_[DASAIMOCHI_SLEEPING][3], 23, 4, 2);
}

void DasaimochiAnim::SetState(DasaimochiState state) {
    if (state < 0 || state >= DASAIMOCHI_STATE_COUNT) {
        state = DASAIMOCHI_IDLE;
    }
    if (current_state_ != state) {
        current_state_ = state;
        current_frame_ = 0;
        blink_counter_ = 0;
        ESP_LOGI(TAG, "Switched Dasaimochi state to %d", static_cast<int>(state));
    }
}

const lv_img_dsc_t* DasaimochiAnim::GetNextFrame() {
    if (!initialized_) {
        Init();
    }

    int frame_to_return = current_frame_;

    if (current_state_ == DASAIMOCHI_IDLE) {
        // Natural blinking algorithm for Idle state
        blink_counter_++;
        if (blink_counter_ < blink_interval_) {
            frame_to_return = 0; // Stay at open eyes
        } else if (blink_counter_ == blink_interval_) {
            frame_to_return = 1; // Half-close
        } else if (blink_counter_ == blink_interval_ + 1) {
            frame_to_return = 2; // Fully closed
        } else if (blink_counter_ == blink_interval_ + 2) {
            frame_to_return = 3; // Reopening
        } else {
            blink_counter_ = 0;
            // Randomize next blink interval between 20 and 45 ticks (2.0s to 4.5s)
            blink_interval_ = 20 + (std::rand() % 25);
            frame_to_return = 0;
        }
    } else {
        // Continuous cycle for active states
        current_frame_ = (current_frame_ + 1) % FRAMES_PER_STATE;
    }

    return &frame_dscs_[current_state_][frame_to_return];
}

DasaimochiState DasaimochiAnim::MapEmotionToState(const char* emotion) {
    if (!emotion || emotion[0] == '\0') return DASAIMOCHI_IDLE;

    std::string str(emotion);
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);

    if (str.find("happy") != std::string::npos || str.find("smile") != std::string::npos ||
        str.find("laugh") != std::string::npos || str.find("excited") != std::string::npos ||
        str.find("joy") != std::string::npos) {
        return DASAIMOCHI_HAPPY;
    }
    if (str.find("thinking") != std::string::npos || str.find("wonder") != std::string::npos ||
        str.find("processing") != std::string::npos) {
        return DASAIMOCHI_THINKING;
    }
    if (str.find("listening") != std::string::npos || str.find("surprised") != std::string::npos ||
        str.find("shocked") != std::string::npos) {
        return DASAIMOCHI_LISTENING;
    }
    if (str.find("speak") != std::string::npos || str.find("talking") != std::string::npos) {
        return DASAIMOCHI_SPEAKING;
    }
    if (str.find("sleep") != std::string::npos || str.find("sad") != std::string::npos ||
        str.find("cry") != std::string::npos || str.find("offline") != std::string::npos) {
        return DASAIMOCHI_SLEEPING;
    }

    return DASAIMOCHI_IDLE;
}

DasaimochiState DasaimochiAnim::MapStatusToState(const char* status) {
    if (!status || status[0] == '\0') return DASAIMOCHI_IDLE;

    std::string str(status);
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);

    if (str.find("listen") != std::string::npos || str.find("hỏi") != std::string::npos || str.find("nghe") != std::string::npos) {
        return DASAIMOCHI_LISTENING;
    }
    if (str.find("speak") != std::string::npos || str.find("talk") != std::string::npos || str.find("nói") != std::string::npos) {
        return DASAIMOCHI_SPEAKING;
    }
    if (str.find("think") != std::string::npos || str.find("nghĩ") != std::string::npos || str.find("chờ") != std::string::npos) {
        return DASAIMOCHI_THINKING;
    }
    if (str.find("sleep") != std::string::npos || str.find("ngủ") != std::string::npos || str.find("standby") != std::string::npos) {
        return DASAIMOCHI_SLEEPING;
    }

    return DASAIMOCHI_IDLE;
}
