#pragma once
#include <stdint.h>

enum {
    MSG_SERVER_JOIN     = 0,   // client -> server: "I want in"
    MSG_CLIENT_ASSIGN   = 1,   // server -> client: "you are player N"
    MSG_SERVER_INPUT    = 2,   // client -> server: per-tick input
    MSG_CLIENT_SNAPSHOT = 3,   // server -> client: per-tick world state
};

typedef struct packet_joinRequest_t{
    uint8_t requestType;
} packet_joinRequest_t;

typedef struct packet_assignRequest_t{
    uint8_t requestType;
    uint8_t player_id;
    
} packet_assignRequest_t;

typedef struct __attribute__((packed)) packet_clientInput_t {
    uint8_t  requestType;
    uint8_t  playerId;
    int16_t axis_X;
    int16_t axis_Y;
    uint8_t  buttons;
} packet_clientInput_t;