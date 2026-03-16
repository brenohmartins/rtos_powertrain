#include <stdio.h>
#include <string.h>
#include "pico/cyw43_arch.h"
#include "lwip/sockets.h"
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include "robot_config.h"
#include "motor_control.h"
#include "wifi_comm.h"

volatile bool wifi_connected = false;

void wifi_manager_task(void *pvParameters) {
    printf("[%s] Initializing WiFi chip...\n", WIFI_MANAGER_TASK);

    if (cyw43_arch_init()) {
        printf("[%s] WiFi init failed! System halted.\n", WIFI_MANAGER_TASK);
        vTaskDelete(NULL);
    }

    cyw43_arch_enable_sta_mode();

    for(;;) {
        if (!wifi_connected) {
            printf("[%s] Connecting to SSID: %s...\n", WIFI_MANAGER_TASK, WIFI_SSID);
 
            if (cyw43_arch_wifi_connect_blocking(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK) == 0) {
                printf("[%s] Connected! IP: %s\n", WIFI_MANAGER_TASK, ip4addr_ntoa(netif_ip4_addr(netif_default)));
                wifi_connected = true;
            } else {
                printf("[%s] Connection failed. Retrying in 3 seconds...\n", WIFI_MANAGER_TASK);
                vTaskDelay(pdMS_TO_TICKS(3000));
            }
        } else {
            int link_status = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);
            if (link_status != CYW43_LINK_UP) {
                printf("[%s] WiFi connection lost!\n", WIFI_MANAGER_TASK);
                wifi_connected = false;
            }

            vTaskDelay(pdMS_TO_TICKS(1000)); 
        }
    }
}

void telemetry_task(void *pvParameters) {
    MotorConfig_t *MotorConfig = (MotorConfig_t *)pvParameters;

    while (!wifi_connected) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(TELEMETRY_PORT);
    dest_addr.sin_addr.s_addr = inet_addr(TELEMETRY_IP);

    const TickType_t xFrequency = pdMS_TO_TICKS(10); 
    TickType_t xLastWakeTime = xTaskGetTickCount();

    float telemetry_data[N_MOTORS];

    printf("[%s] Sending to %s:%d\n", TELEMETRY_TASK, TELEMETRY_IP, TELEMETRY_PORT);

    for( ; ; ) {
        for (int i = 0; i < N_MOTORS; i++) {
            if(xQueueReceive(MotorConfig[i].telemetry_queue, &telemetry_data[i], 0U)!= pdPASS) {
                telemetry_data[i] = 0.0f;
            }
        }
        sendto(sock, telemetry_data, sizeof(telemetry_data), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void setpoint_task(void *pvParameters) {
    MotorConfig_t *motors = (MotorConfig_t *)pvParameters;

    while (!wifi_connected) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    struct sockaddr_in server_addr;
    int sock = socket(AF_INET, SOCK_DGRAM, 0); 
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SETPOINT_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));

    uint8_t buffer[128]; 
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    printf("[%s] Listening for setpoints on UDP port %d\n", SETPOINT_TASK, SETPOINT_PORT);

    for ( ; ; ) {
        int len = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &addr_len);
                           
        if (len > 0) {
            if (len % sizeof(float) != 0) {
                printf("[%s] Ignored corrupted packet: %d bytes is not a clean float array.\n", SETPOINT_TASK, len);
                continue; 
            }

            int num_received_setpoints = len / sizeof(float);
            float *received_setpoints = (float *)buffer;

            if (num_received_setpoints == N_MOTORS) {
                
                printf("[%s] ", SETPOINT_TASK);
                for (int i = 0; i < N_MOTORS; i++) {
                    printf("M%d: %.2f | ", i, received_setpoints[i]);
                    
                    xQueueSend(motors[i].setpoint_queue, &received_setpoints[i], 0U);
                }
                printf("\n");

            } else {
                printf("[%s] Size Mismatch! Expected %d floats, got %d.\n", SETPOINT_TASK, N_MOTORS, num_received_setpoints);
            }
        }
    }
}