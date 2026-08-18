//
// Created by Hannah on 02.06.2025.
//

#if IsDebug
#include "../header/debug.h"
#include <stdio.h>

void Debug_PrintSplitString(const char** ptr_CharArray_C) {
    if (ptr_CharArray_C == NULL) {
        log("PrintSplitString: ptr_charArray_C is NULL");
        return;
    }
    int counter = 0;
    while (*ptr_CharArray_C != NULL) {
        printf("String %i: %s\n", counter, *ptr_CharArray_C);
        ++counter;
        ++ptr_CharArray_C;
    }
}

#endif