// uart_pi.h
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    /* Shared robot control from app_main.c */
    void robot_set_direction(char cmd);
    void robot_set_target_speed(int pps);
    void robot_stop(void);

    /* Shared wheel speed from app_main.c */
    extern int g_speed_left_pps;
    extern int g_speed_right_pps;

    /* Shared encoder delta from app_main.c */
    extern int g_delta_left_count;
    extern int g_delta_right_count;

    /* Link with Pi over USB serial */
    void pi_link_init(void);
    void pi_link_reset_odom(void);
    void send_echo(const char *msg);
    void pi_uart_task(void *arg);

    /* Odom */
    void pi_link_update_odom_from_counts(int delta_left_count, int delta_right_count, float dt);
    void pi_link_periodic(void);

    /* Telnet server */
    void telnet_server_start(void);

#ifdef __cplusplus
}
#endif