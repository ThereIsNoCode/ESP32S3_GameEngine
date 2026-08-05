#pragma once
#include <stdint.h>

#define LVL_ID_TESTLEVEL 0
#define MAX_LEVEL_SIZE 33*15
//In units of tiles 
extern uint16_t lvl_width;
extern uint16_t lvl_height;

extern uint8_t lvl_data[MAX_LEVEL_SIZE];

void LoadLevel(uint16_t level_id);

