#pragma once

#include <string.h>

#define MAX_NOTIFICATIONS 10
extern uint8_t notification_index;

typedef struct
{
    uint32_t NotificationUID;
    char Identifier[64];
    char Title[64];
    char Subtitle[64];
    char Message[256];
} NotificationAttributes;

// Define the callback type
typedef void (*notification_callback_t)(NotificationAttributes *notification);

// Function to receive notification data
void esp_receive_apple_data_source(uint8_t *message, uint16_t message_len);

void ancs_app(notification_callback_t callback);