#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <FreeRTOS.h>
#include <queue.h>
#include <stdint.h>

// motors structure
typedef struct {
    int motor_id;
    float target_speed;
    float measured_speed;
    float alpha;
    float Kp;
    float Ki;
    float Kd;
    float dt;
    int dt_ms;
    volatile long encoder_ticks;
    uint enc_a_pin;
    uint enc_b_pin;
    uint pwm_fwd_pin;
    uint pwm_rev_pin;
    uint pwm_slice_fwd;
    uint pwm_slice_rev;
    QueueHandle_t setpoint_queue;
    QueueHandle_t telemetry_queue;
} MotorConfig_t;

// global array motors
extern MotorConfig_t MotorControls[N_MOTORS];

// public funtions
void init_motor_hardware(void);
void encoder_isr(uint gpio, uint32_t events);
void motor_controller_task(void *pvParameters);

#endif