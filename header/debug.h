//
// Created by Hannah on 02.06.2025.
//

#ifndef DEBUG_H
#define DEBUG_H
#if IsDebug

#include "essentials.h"
#define log(msg) printf("Debug: %s\n", msg);

/**
 * @brief Prints an array of null-terminated strings, each on a new line, along with an index.
 *
 * This function iterates through an array of null-terminated strings (provided as
 * a double pointer) and prints each string along with its index. The function stops
 * processing upon encountering a NULL pointer within the array.
 *
 * @param ptr_CharArray_C Pointer to an array of null-terminated strings. The array
 *        must have a NULL pointer as the last element to serve as a terminator.
 *        Passing a NULL value for this parameter will result in a debug log and
 *        the function returning immediately.
 */
void Debug_PrintSplitString(const char** ptr_CharArray_C);

#else
#define log(msg)

#endif
#endif