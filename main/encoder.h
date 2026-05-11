#ifndef ENCODER_H
#define ENCODER_H

#ifdef __cplusplus
extern "C" {
#endif

// Khởi tạo PCNT cho 4 Encoders
void encoder_init(void);

// Đọc giá trị xung tích lũy hiện tại của 4 động cơ
void encoder_get_counts(int *count_fl, int *count_rl, int *count_fr, int *count_rr);

// Reset toàn bộ đếm xung về 0
void encoder_clear_counts(void);

#ifdef __cplusplus
}
#endif

#endif // ENCODER_H