#pragma once
#include <stdint.h>
#include "../entities/bomb.h"

//bit 0 of uint8_t
#define NET_BTN_JUMP (1 << 0)
//bit 1 of uint8_t
#define NET_INPUT_INTERACT (1 << 1)

#define NET_INPUT_PRESSED(buttonsByte, btnMask)  (((buttonsByte) & (btnMask)) != 0)

enum {
    MSG_SERVER_JOIN     = 0,   // client -> server: Tells Server that client wants to join game
    MSG_CLIENT_ASSIGN   = 1,   // server -> client: Tells Client, you are THIS user (in my case, Id = 1 / player2)
    MSG_SERVER_INPUT    = 2,   // client -> server: per-tick input 
    MSG_CLIENT_SNAPSHOT = 3,   // server -> client: per-tick world state
    MSG_CLIENT_INPUT    = 4,   // server -> client: this is host's current input to client for clientside prediction to work
};

typedef struct packet_joinRequest_t{
    uint8_t requestType;
} packet_joinRequest_t;

typedef struct packet_assignRequest_t{
    uint8_t requestType;
    uint8_t player_id;
    
} packet_assignRequest_t;

//If we want to do Server Authorative, we need to compute everything on HOST
//Maybe I will do this when I write a PC port of the game, or the server code on PC :)
typedef struct __attribute__((packed)) packet_clientInput_t {
    uint8_t  requestType;
    uint8_t  playerId;
    int16_t axis_X;
    int16_t axis_Y;
    uint8_t  buttons;
} packet_clientInput_t;

//If we want to do Client Authorative, we just send all client position
//Risky if people can modify their position, but since its a LAN, for fun project,
//I don't see why not :)
typedef struct __attribute__((packed)) packet_clientPosition_t {
    uint8_t  requestType;
    uint8_t  playerId;
    int32_t position_X;
    int32_t position_Y;
    //Probably another state for sprite 
} packet_clientPosition_t;

//Before Sequencing
typedef struct __attribute__((packed)) packet_serverSnapshot_t {
    uint8_t  requestType;
    EntityPacket players[2];
    EntityPacket bombs[5];
    //what hsould be here
} packet_serverSnapshot_t;

// //After Sequencing
// typedef struct __attribute__((packed)) packet_serverSnapshot_t {
//     uint8_t  requestType;
//     uint32_t seq; //ensures old packets will not be computed
//     EntityPacket players[2];
//     EntityPacket bombs[5];

// } packet_serverSnapshot_t;