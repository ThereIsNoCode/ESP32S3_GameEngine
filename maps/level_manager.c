#include "level_manager.h"
#include "TestLevel.h"

uint16_t lvl_width;
uint16_t lvl_height;
uint8_t lvl_data[MAX_LEVEL_SIZE];

void LoadLevel(uint16_t level_id){
    switch(level_id){
        case LVL_ID_TESTLEVEL:
            lvl_width = LVL_TESTLEVEL_WIDTH;
            lvl_height = LVL_TESTLEVEL_HEIGHT;
            uint32_t tileCount = (lvl_width*lvl_height);
            for(int i = 0; i < tileCount; i++){
                lvl_data[i] = TestLevel_tiles[i];
            }
            break;
    }
}