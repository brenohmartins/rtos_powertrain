#ifndef ROBOT_CONFIG_H
#define ROBOT_CONFIG_H

#include <FreeRTOS.h>
#include <stdbool.h>

// wifi configuration
#define SETPOINT_TASK "WIFI_SETPOINT"
#define TELEMETRY_TASK "WIFI_TELEMETRY"
#define WIFI_MANAGER_TASK "WIFI_MANAGER"
#define WIFI_SSID "Quarto"
#define WIFI_PASSWORD "07055492"
#define SETPOINT_PORT 1234
#define TELEMETRY_PORT 4321
#define TELEMETRY_IP "192.168.1.104"

// mtor configuration
#define MOTOR_CONTROLLER_TASK "MOTOR_CONTROLLER"
#define PWM_FREQ 20000
#define CLK_DIV configCPU_CLOCK_HZ / 100000000.0f
#define PWM_WRAP 100000000 / PWM_FREQ
#define N_MOTORS 2
#define TICKS_PER_REV 360.0f 
#define LEFT 0
#define RIGHT 1

// global variables
extern volatile bool wifi_connected;

#endif