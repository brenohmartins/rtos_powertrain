#ifndef WIFI_COMM_H
#define WIFI_COMM_H

#include "motor_control.h"

// public funtions WiFi-tasks

void wifi_manager_task(void *pvParameters);
void telemetry_task(void *pvParameters);
void setpoint_task(void *pvParameters);

#endif