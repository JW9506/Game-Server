#pragma once;
#include "mdump.h"
#include <stdio.h>

void mdump(void* ptr, int len) {
    printf("\n");
    for (int i = 0; i < len; i++) {
        if (i % 8 == 0 && i != 0) printf(" ");
        if (i % 16 == 0 && i != 0) printf("\n");
        printf("%02x ", *((unsigned char*)ptr + i));
    }
    printf("\n");
}
