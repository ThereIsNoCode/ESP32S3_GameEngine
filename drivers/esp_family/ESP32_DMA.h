#pragma once
#include <stdio.h>

typedef struct {
    uint32_t dw0;
    uint32_t buf;
    uint32_t next;
} dma_desc_t;