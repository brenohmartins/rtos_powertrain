#include <stdio.h>
#include "pico/stdlib.h"
#include <FreeRTOS.h>
#include <task.h>

#include "robot_config.h"
#include "motor_control.h"
#include "wifi_comm.h"

int main() {
    stdio_init_all();
    
    init_motor_hardware();
    
    // Usando os valores ORIGINAIS do seu código
    xTaskCreate(motor_controller_task, MOTOR_CONTROLLER_TASK, 512, 
                &MotorControls[LEFT], 4, NULL);
    xTaskCreate(motor_controller_task, MOTOR_CONTROLLER_TASK, 512, 
                &MotorControls[RIGHT], 4, NULL);
    xTaskCreate(wifi_manager_task, WIFI_MANAGER_TASK, 1024, 
                NULL, 3, NULL);
    xTaskCreate(setpoint_task, SETPOINT_TASK, 1024, 
                (void*)MotorControls, 2, NULL);
    xTaskCreate(telemetry_task, TELEMETRY_TASK, 1024, 
                (void*)MotorControls, 1, NULL);

    vTaskStartScheduler();

    for( ; ; ) {}

    return 0;
}