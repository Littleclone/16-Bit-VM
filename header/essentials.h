/*
 * MIT License
 * Copyright (c) 2025 Littleclone
 * Permission is granted to use, copy, modify, and distribute this software,
 * provided that the copyright notice and this permission notice are included.
 * The software is provided "as is", without warranty of any kind.
 */
#ifndef essentials_h
#define essentials_h

#include <stdbool.h>
#include "raylib.h"
#include "debug.h"

typedef unsigned char byte;
typedef char sbyte;

/**
 * @brief Represents a node in a linked list of arrays, where each node contains an array and a reference to the next node.
 *
 * This structure is designed to manage elements in contiguous blocks (arrays) while linking them together for sequence traversal.
 * It allows dynamic memory allocation with linked connections between segments of data.
 */
typedef struct LinkedArray {
    /**
     * @brief A pointer to an array within the LinkedArray structure.
     *
     * This field holds a dynamically allocated array or sequence of elements.
     * The specific type of data stored in the array is not defined here and
     * depends on the implementation or usage context.
     */
    void* array;
    /**
     * @brief Pointer to the next LinkedArray structure in the linked list.
     *
     * This member provides a connection to the subsequent LinkedArray, enabling
     * the creation of a linked list of arrays.
     */
    struct LinkedArray* next;
    /**
     * @brief Represents the size of the array in the LinkedArray structure.
     *
     * This member stores the number of elements within the array managed by the LinkedArray structure.
     * It is defined using the byte datatype, which is an unsigned 8-bit value.
     */
    byte arraySize;
}LinkedArray;


/**
 * @brief Determines if the given character is an alphabetic character (either uppercase or lowercase).
 *
 * @param C_character The character to check, represented as an unsigned char.
 * @return True if the character is alphabetic (A-Z or a-z), otherwise false.
 */
bool P_IsChar(char C_character);

/**
 * Checks if the provided character is a digit (0-9).
 *
 * @param C_character The character to be checked.
 * @return true if the character is a digit, false otherwise.
 */
bool P_IsDigit(char C_character);

/**
 * Checks if a given character is an uppercase letter.
 *
 * @param C_character The character to be checked.
 * @return Returns true if the character is an uppercase letter (A-Z), otherwise false.
 */
bool P_IsUpper(char C_character);

/**
 * @brief Checks if a given character is a lowercase alphabetical letter.
 *
 * This function determines whether the specified character falls within
 * the range of lowercase ASCII letters ('a' to 'z').
 *
 * @param C_character The character to check, represented as an unsigned char.
 * @return Returns true if the character is a lowercase letter, false otherwise.
 */
bool P_IsLower(char C_character);

/**
 * Converts all lowercase alphabetic characters in a string to uppercase.
 *
 * @param string A pointer to the null-terminated string that will be modified in-place.
 */
void P_ToUpper(char* string);

/**
 * Converts all uppercase characters in a given string to their lowercase equivalents.
 *
 * @param string The string to be transformed. Must be null-terminated. Modifies the string in place.
 */
void P_ToLower(char* string);

/**
 * @brief Converts an unsigned short integer to a dynamically allocated string representation.
 *
 * This function takes an unsigned short integer value and returns its string representation.
 * The returned string is allocated on the heap, and it is the caller's responsibility to free the memory.
 *
 * @param value The unsigned short integer to convert to a string.
 * @return A pointer to the dynamically allocated string representing the input value, or NULL if memory allocation fails.
 */
char* P_ToString(unsigned short value);

/**
 * @brief Calculates the length of the given string.
 *
 * This function computes the number of characters in the null-terminated string
 * pointed to by the input parameter.
 *
 * @param string_C A pointer to the null-terminated string whose length is to be calculated.
 *               The string must not be NULL.
 * @return The length of the string (number of characters before the null terminator).
 */
unsigned int P_StrLen(const char* string_C);

/**
 * @brief Calculates the length of a C-style string starting from a specified position.
 *
 * This function determines the number of characters in the specified string
 * from the given starting position up to the null terminator.
 *
 * @param C_string_C A constant pointer to the C-style string for which the length is to be calculated.
 * @param pos The starting position within the string from which to begin counting the length.
 * @return The length of the string starting from the specified position.
 */
unsigned int P_StrLenBeginAt(const char* const C_string_C, unsigned short pos);

/**
 * Concatenates two null-terminated strings and returns the newly created string.
 * The resulting string will contain the contents of the first string followed by the contents of the second string.
 * Memory for the new string is dynamically allocated and must be freed by the caller.
 *
 * @param string_C The first null-terminated string.
 * @param stringToAdd_C The second null-terminated string to append to the first string.
 * @return A pointer to the newly created null-terminated string, or nullptr if memory allocation fails.
 */
char* P_StrAdd(const char* string_C, const char* stringToAdd_C);

/**
 * @brief Removes all characters from the specified string up to and including the specified character.
 *
 * This function creates a new string by discarding all the characters from the beginning
 * of the given string until and including the specified character. The resulting string
 * is dynamically allocated and must be freed by the caller to avoid memory leaks.
 *
 * @param string_C The source string from which characters will be removed.
 * @param C_removeUntil The character until which the removal occurs, including the character itself.
 * @return A new string excluding characters up to and including the specified character, or NULL if memory allocation fails.
 */
char* P_StrRmv(const char* string_C, const char C_removeUntil);

/**
 * Creates a copy of the given null-terminated string.
 * Allocates memory for the new string and copies the content of the input string into it.
 * The caller is responsible for freeing the allocated memory.
 *
 * @param string_C A pointer to a null-terminated string to be copied.
 * @return A pointer to the newly allocated string containing the copied content,
 *         or NULL if the memory allocation fails.
 */
char* P_StrCpy(const char* string_C);

/**
 * @brief Compares two null-terminated strings for equality.
 *
 * @param string1_C Pointer to the first null-terminated string to compare.
 * @param string2_C Pointer to the second null-terminated string to compare.
 * @return True if both strings are identical, otherwise false.
 */
bool P_StrCmp(const char* string1_C, const char* string2_C);

/**
 * @brief Checks if a given substring exists within a string.
 *
 * @param string_C The main string to search within, represented as a null-terminated array of unsigned characters.
 * @param stringHas_C The substring to search for, represented as a null-terminated array of unsigned characters.
 * @return True if the substring is found within the main string, otherwise false.
 */
bool P_StrHas(const char* string_C, const char* stringHas_C);

/**
 * @brief Splits the input string into an array of dynamically allocated strings, using a specified delimiter.
 *
 * @param string_C The input string to be split, represented as a pointer to an unsigned char array.
 *                 Must be null-terminated.
 * @param C_split The delimiter character used to divide the string into segments.
 * @return A dynamically allocated array of unsigned char pointers, where each pointer points to a
 *         null-terminated substring split from the original string. The final element in the array
 *         will be a null pointer. Returns NULL if memory allocation fails.
 */
char** P_StrSplit(const char* string_C, char C_split);

/**
 * @brief Creates a new LinkedArray with a specified size and multiplication factor.
 *
 * This function dynamically allocates memory for a LinkedArray structure and its associated array.
 * The size of the array is determined by multiplying the provided size and factor.
 *
 * @param arraySize_C The base size of the array to be created. Represented as an unsigned 8-bit value (byte).
 * @param byteFactor_C The multiplication factor to determine the final array size. Represents how large the
 *               array will be relative to the base size.
 * @return A pointer to the newly created LinkedArray structure if successful, or NULL if memory allocation fails.
 */
LinkedArray* P_CreateLinkedArray(byte arraySize_C, byte byteFactor_C);

/**
 * @brief Adds a new linked array element to the end of the linked list.
 *
 * This function appends a new element to a linked list of LinkedArray structures. It dynamically allocates
 * memory for both the LinkedArray structure and its associated array, initializes them, and maintains
 * the sequence by connecting the new element to the existing list. If memory allocation fails, appropriate
 * cleanup is performed, and the function returns a failure status.
 *
 * @param headArray Pointer to the head of the linked list of LinkedArray structures.
 * @param arraySize_C The size of each element in the newly allocated array.
 * @param byteFactor_C The number of elements to allocate in the new array.
 * @return Returns true if the new element is successfully added, or false if memory allocation fails.
 */
bool P_AddLinkedArrayElement(LinkedArray* headArray, byte arraySize_C, byte byteFactor_C);

/**
 * @brief Deallocates memory associated with a linked list of LinkedArray structures.
 *
 * This function traverses a linked list of LinkedArray structures and frees the memory
 * allocated for their internal arrays and the structures themselves. After calling this
 * function, the entire linked list is cleared and any dangling pointers are removed.
 *
 * @param headArray A pointer to the head of the linked list of LinkedArray structures.
 *                  It may be NULL, in which case the function performs no operation.
 */
void P_ClearLinkedArray(LinkedArray* headArray);


byte P_IsRegister(char* input);
unsigned short P_ValidateOpCode(char* input);

#endif // !essentials_h

