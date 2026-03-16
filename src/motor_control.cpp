#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include "robot_config.h"
#include "motor_control.h"

// Definição do array global de motores
MotorConfig_t MotorControls[N_MOTORS];

void encoder_isr(uint gpio, uint32_t events) {
    for (int i = 0; i < N_MOTORS; i++) {
        if (gpio == MotorControls[i].enc_a_pin || gpio == MotorControls[i].enc_b_pin) {
            
            bool a_state = gpio_get(MotorControls[i].enc_a_pin);
            bool b_state = gpio_get(MotorControls[i].enc_b_pin);

            if (gpio == MotorControls[i].enc_a_pin) {
                if (a_state == b_state) {
                    MotorControls[i].encoder_ticks++;
                } else {
                    MotorControls[i].encoder_ticks--;
                }
            } else if (gpio == MotorControls[i].enc_b_pin) {
                if (a_state == b_state) {
                    MotorControls[i].encoder_ticks--;
                } else {
                    MotorControls[i].encoder_ticks++;
                }
            }
            
            break;
        }
    }
}

void motor_controller_task(void *pvParameters) {
    MotorConfig_t *MotorConfig = (MotorConfig_t *)pvParameters;
    
    long previous_ticks = 0;
    float received_setpoint = 0.0f;

    float error = 0.0f;
    float previous_error = 0.0f;
    float accumulated_error = 0.0f;
    const float integral_max = 50.0f; 
    const float integral_min = -50.0f;

    const TickType_t xFrequency = pdMS_TO_TICKS(MotorConfig->dt_ms); 
    TickType_t xLastWakeTime = xTaskGetTickCount();

    printf("[%s_%d] Initializing Controller...\n", MOTOR_CONTROLLER_TASK, MotorConfig->motor_id);

    for( ; ; ) {
        if(xQueueReceive(MotorConfig->setpoint_queue, &received_setpoint, 0U) == pdPASS) {
            MotorConfig->target_speed = received_setpoint;
        }

        long current_ticks = MotorConfig->encoder_ticks;
        long delta_ticks = current_ticks - previous_ticks;
        previous_ticks = current_ticks;

        float revolutions = (float)delta_ticks / TICKS_PER_REV;
        float raw_speed = (revolutions * 2.0f * M_PI) / MotorConfig->dt;
        MotorConfig->measured_speed = (MotorConfig->alpha * raw_speed) + ((1.0f - MotorConfig->alpha) * MotorConfig->measured_speed);

        xQueueSend(MotorConfig->telemetry_queue, &MotorConfig->measured_speed, 0U);
        
        error = MotorConfig->target_speed - MotorConfig->measured_speed;
        float P = MotorConfig->Kp * error;
        accumulated_error = accumulated_error + (error * MotorConfig->dt);
        if (accumulated_error > integral_max) {
            accumulated_error = integral_max;
        } else if (accumulated_error < integral_min) {
            accumulated_error = integral_min;
        }
        float I = MotorConfig->Ki * accumulated_error;
        float D = MotorConfig->Kd * (error - previous_error) / MotorConfig->dt;
        previous_error = error;
        float control_signal = P + I + D;

        if (control_signal > 100.0f) control_signal = 100.0f;
        if (control_signal < -100.0f) control_signal = -100.0f;

        uint16_t duty_cycle = (uint16_t)(fabs(control_signal) * PWM_WRAP / 100.0f);

        if (control_signal > 0.0f) {
            pwm_set_gpio_level(MotorConfig->pwm_fwd_pin, duty_cycle);
            pwm_set_gpio_level(MotorConfig->pwm_rev_pin, 0);
            
        } else if (control_signal < 0.0f) {
            pwm_set_gpio_level(MotorConfig->pwm_fwd_pin, 0);
            pwm_set_gpio_level(MotorConfig->pwm_rev_pin, duty_cycle);
            
        } else {
            pwm_set_gpio_level(MotorConfig->pwm_fwd_pin, 0);
            pwm_set_gpio_level(MotorConfig->pwm_rev_pin, 0);
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void init_motor_hardware() {
    int dt_ms = 10;
    float dt = (float)dt_ms / 1000.0f;
    float alpha = 0.25f;
    float Kp = 5.0f;
    float Ki = 4.0f;
    float Kd = 0.0f;
    
    MotorControls[LEFT].motor_id = LEFT;
    MotorControls[LEFT].enc_a_pin = 10;
    MotorControls[LEFT].enc_b_pin = 9;
    MotorControls[LEFT].pwm_fwd_pin = 2;
    MotorControls[LEFT].pwm_rev_pin = 3;
    MotorControls[LEFT].alpha = alpha;
    MotorControls[LEFT].Kp = Kp; 
    MotorControls[LEFT].Ki = Ki;
    MotorControls[LEFT].Kd = Kd;
    MotorControls[LEFT].dt_ms = dt_ms;
    MotorControls[LEFT].dt = dt;

    MotorControls[RIGHT].motor_id = RIGHT;
    MotorControls[RIGHT].enc_a_pin = 6;
    MotorControls[RIGHT].enc_b_pin = 5;
    MotorControls[RIGHT].pwm_fwd_pin = 0;
    MotorControls[RIGHT].pwm_rev_pin = 1;
    MotorControls[RIGHT].alpha = alpha;
    MotorControls[RIGHT].Kp = Kp; 
    MotorControls[RIGHT].Ki = Ki;
    MotorControls[RIGHT].Kd = Kd;
    MotorControls[RIGHT].dt_ms = dt_ms;
    MotorControls[RIGHT].dt = dt;

    for(int i = 0; i < N_MOTORS; i++) {
        MotorControls[i].setpoint_queue = xQueueCreate(1, sizeof(float));
        MotorControls[i].telemetry_queue = xQueueCreate(1, sizeof(float));
        MotorControls[i].encoder_ticks = 0;

        gpio_init(MotorControls[i].enc_a_pin);
        gpio_set_dir(MotorControls[i].enc_a_pin, GPIO_IN);
        gpio_pull_up(MotorControls[i].enc_a_pin);
        
        gpio_init(MotorControls[i].enc_b_pin);
        gpio_set_dir(MotorControls[i].enc_b_pin, GPIO_IN);
        gpio_pull_up(MotorControls[i].enc_b_pin);

        if (i == 0) {
            gpio_set_irq_enabled_with_callback(
                MotorControls[i].enc_a_pin, 
                GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                true, 
                &encoder_isr
            );
        } else {
            gpio_set_irq_enabled(
                MotorControls[i].enc_a_pin, 
                GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                true
            );
        }

        gpio_set_irq_enabled(MotorControls[i].enc_b_pin, 
                             GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                             true);

        gpio_set_function(MotorControls[i].pwm_fwd_pin, GPIO_FUNC_PWM);
        gpio_set_function(MotorControls[i].pwm_rev_pin, GPIO_FUNC_PWM);

        MotorControls[i].pwm_slice_fwd = pwm_gpio_to_slice_num(MotorControls[i].pwm_fwd_pin);
        MotorControls[i].pwm_slice_rev = pwm_gpio_to_slice_num(MotorControls[i].pwm_rev_pin);

        pwm_set_clkdiv(MotorControls[i].pwm_slice_fwd, CLK_DIV);
        pwm_set_clkdiv(MotorControls[i].pwm_slice_rev, CLK_DIV);

        pwm_set_wrap(MotorControls[i].pwm_slice_fwd, PWM_WRAP);
        pwm_set_wrap(MotorControls[i].pwm_slice_rev, PWM_WRAP);

        pwm_set_gpio_level(MotorControls[i].pwm_fwd_pin, 0);
        pwm_set_gpio_level(MotorControls[i].pwm_rev_pin, 0);

        pwm_set_enabled(MotorControls[i].pwm_slice_fwd, true);
        pwm_set_enabled(MotorControls[i].pwm_slice_rev, true);
    }
}