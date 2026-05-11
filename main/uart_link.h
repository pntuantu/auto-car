#ifndef UART_LINK_H
#define UART_LINK_H

#ifdef __cplusplus
extern "C" {
#endif

// Khởi tạo kênh giao tiếp USB Serial JTAG
void uart_link_init(void);

// Task nhận lệnh vận tốc từ Pi và Task gửi Odom lên Pi
void uart_tx_task(void *arg);
void uart_rx_task(void *arg);

#ifdef __cplusplus
}
#endif

#endif // UART_LINK_H