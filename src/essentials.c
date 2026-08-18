/*
 * MIT License
 * Copyright (c) 2025 Littleclone
 * Permission is granted to use, copy, modify, and distribute this software,
 * provided that the copyright notice and this permission notice are included.
 * The software is provided "as is", without warranty of any kind.
 */
#include "../header/essentials.h"
#include <stdio.h>
#include <stdlib.h>
#include "../header/CPU.h"

bool P_IsChar(const char C_character) {
    return (C_character >= 'a' && C_character <= 'z') || (C_character >= 'A' && C_character <= 'Z');
}
bool P_IsDigit(const char C_character) {
    return C_character >= '0' && C_character <= '9';
}
bool P_IsUpper(const char C_character) {
    return C_character >= 'A' && C_character <= 'Z';
}
bool P_IsLower(const char C_character) {
    return C_character >= 'a' && C_character <= 'z';
}
void P_ToUpper(char* string) {
    while (*string != '\0') {
        if (P_IsLower(*string)) {
            *string -= 32;
        }
        ++string;
    }
}
void P_ToLower(char* string) {
    while (*string != '\0') {
        if (P_IsUpper(*string)) {
            *string += 32;
        }
        ++string;
    }
}
char *P_ToString(unsigned short value) {
	// Fix: Initialize numCounter to 0 for proper counting
	unsigned short numCounter = 0;
	unsigned short tempValue = value;

	// Fix: Handle special case when value is 0
	if (value == 0) {
		numCounter = 1;
	} else {
		// Count number of digits
		while (tempValue != 0) {
			tempValue /= 10;
			++numCounter;
		}
	}

	// Allocate memory for string plus null terminator
	char *stringValue = malloc(numCounter + 1);
	if (stringValue == NULL) {
		log("Fehler bei ToString");
		return NULL;
	}

	// Fix: Use size of unsigned short for numbers array
	unsigned short *numbers = malloc(numCounter * sizeof(unsigned short));
	if (numbers == NULL) {
		free(stringValue);
		return NULL;
	}

	// Fix: Reset numCounter and store digits
	numCounter = 0;
	tempValue = value;

	// Fix: Handle special case when value is 0
	if (value == 0) {
		numbers[0] = 0;
		numCounter = 1;
	} else {
		do {
			numbers[numCounter] = tempValue % 10;
			tempValue /= 10;
			++numCounter;
		} while (tempValue != 0);
	}

	// Fix: Use separate index for string position
	unsigned short pos = 0;
	for (sbyte i = numCounter - 1; i >= 0; --i) {
		stringValue[pos] = numbers[i] + '0';
		++pos;
	}

	// Add null terminator
	stringValue[pos] = '\0';

	free(numbers);
	return stringValue;
}

unsigned int P_StrLen(const char* string_C) {
    unsigned int counter = 0;
    while (*string_C != '\0') {
        ++counter;
        ++string_C;
    }
    return counter;
}
unsigned int P_StrLenBeginAt(const char* const C_string_C, unsigned short pos) {
	unsigned int counter = 0;
	while (C_string_C[pos] != '\n') {
		++counter;
		++pos;
	}
	return counter;
}
char* P_StrAdd(const char* string_C, const char* stringToAdd_C) {
    const unsigned int C_stringLenght = P_StrLen(string_C);
    const unsigned int C_stringToAddLenght = P_StrLen(stringToAdd_C);
    char* const C_newString = malloc(C_stringLenght + C_stringToAddLenght + 1);
    if (C_newString == NULL) {
        return NULL;
    }
    char* tempStringPtr = C_newString;
    while (*string_C != '\0') {
        *tempStringPtr = *string_C;
        ++tempStringPtr;
        ++string_C;
    }
    while (*stringToAdd_C != '\0') {
        *tempStringPtr = *stringToAdd_C;
        ++tempStringPtr;
        ++stringToAdd_C;
    }
    *tempStringPtr = '\0';
    return C_newString;
}
char* P_StrRmv(const char* string_C, const char C_removeUntil) {
    unsigned int stringLenght = P_StrLen(string_C);
    char* temp = (char*)string_C;
    temp += stringLenght;
    while (*temp != C_removeUntil) {
        --temp;
        --stringLenght;
    }
    char* newString = malloc(stringLenght);
    if (newString == NULL) {
        log("Error bei newString")
        return NULL;
    }
    newString[stringLenght] = '\0';
    temp = newString;
    byte index = 0;
    while (index != stringLenght) {
        *temp = *string_C;
        ++temp;
        ++string_C;
        ++index;
    }
    return newString;
}
char* P_StrCpy(const char* string_C) {
    const unsigned int C_stringLenght = P_StrLen(string_C);
    char* const C_newString = malloc(C_stringLenght + 1);
    if (C_newString == NULL) {
        return NULL;
    }
    char* tempString = C_newString;
    while (*string_C != '\0') {
        *tempString = *string_C;
        ++tempString;
        ++string_C;
    }
    return C_newString;
}
bool P_StrCmp(const char* string1_C, const char* string2_C) {
    while (1) {
        if (*string1_C == '\0' && *string2_C == '\0') {
            return true;
        }
        if (*string1_C == *string2_C) {
            ++string1_C;
            ++string2_C;
        }
        else {
            return false;
        }
    }
}
bool P_StrHas(const char* string_C, const char* stringHas_C) {
    int counter = 0;
    while (*string_C != '\0') {
        if (*string_C == *stringHas_C) {
            ++stringHas_C;
            if (*stringHas_C == '\0') {
                return true;
            }
            ++counter;
        }
        else {
            stringHas_C -= counter;
            counter = 0;
        }
        ++string_C;
    }
    return false;
}
char** P_StrSplit(const char* string_C, const char C_split) {
    struct stringNode {
        char* string;
        struct stringNode* next;
    };
    struct stringNode* placeholder = NULL;
    struct stringNode** head_ptr = &placeholder;
    int nodeCounter = 0;
    bool error_Happened = false;
    const char* tempString_C = string_C;
    while (*tempString_C != '\0') {
        int counter = 0;
        while (*tempString_C != C_split && *tempString_C != '\0') {
            ++counter;
            ++tempString_C;
        }
        tempString_C -= counter;
        char* newString = malloc(counter + 1);
        struct stringNode* newNode = malloc(sizeof(struct stringNode));
        if (newString == NULL || newNode == NULL) {
            error_Happened = true;
            break;
        }
        newNode->next = NULL;
        ++nodeCounter;
        counter = 0;
        while (*tempString_C != C_split && *tempString_C != '\0') {
            newString[counter] = *tempString_C;
            ++counter;
            ++tempString_C;
        }
        if (*tempString_C != '\0') {
            ++tempString_C;
        }
        newString[counter] = '\0';
        newNode->string = newString;
        if (*head_ptr != NULL) {
            struct stringNode* temp = *head_ptr;
            while (temp->next != NULL) {
                temp = temp->next;
            }
            temp->next = newNode;
        } else {
            *head_ptr = newNode;
        }
    }
    char** ptr_Array = malloc((nodeCounter + 1)* sizeof(char*));
    if (ptr_Array == NULL) {
        error_Happened = true;
    }
    if (error_Happened && *head_ptr != NULL) {
        struct stringNode* temp = *head_ptr, *temp2 = NULL;
        while (temp != NULL) {
            temp2 = temp;
            free(temp->string);
            temp = temp->next;
            free(temp2);
        }
        return NULL;
    }
    if (*head_ptr != NULL && ptr_Array != NULL) {
        struct stringNode* temp = *head_ptr, *temp2 = NULL;
        int index = 0;
        while (temp != NULL) {
            temp2 = temp;
            temp = temp->next;
            ptr_Array[index] = temp2->string;
            ++index;
            free(temp2);
        }
        head_ptr = NULL;
        ptr_Array[index] = NULL;
    }
    return ptr_Array;
}

// Linked Array

LinkedArray* P_CreateLinkedArray(const byte arraySize_C, const byte byteFactor_C) {
	LinkedArray* linkedArray = malloc(sizeof(LinkedArray));
	if (linkedArray == NULL) {
		log("Linked Array konnte nicht erstellt werden.")
		return NULL;
	}
	linkedArray->array = malloc(arraySize_C * byteFactor_C);
	if (linkedArray->array == NULL) {
		log("Array fürs Linked Array konnte nicht erstellt werden.")
		free(linkedArray);
		return NULL;
	}
	linkedArray->arraySize = arraySize_C;
	linkedArray->next = NULL;
	return linkedArray;
}

bool P_AddLinkedArrayElement(LinkedArray* headArray, const byte arraySize_C, const byte byteFactor_C) {
	LinkedArray* newElement = malloc(sizeof(LinkedArray));
	if (newElement == NULL) {
		log("Neues Element konnte nicht erstellt werden.")
		return false;
	}
	void* newArray = malloc(arraySize_C * byteFactor_C);
	if (newArray == NULL) {
		log("NewArray konnte nicht erstellt werden.")
		free(newElement);
		return false;
	}
	newElement->arraySize = arraySize_C;
	newElement->array = newArray;
	newElement->next = NULL;
	while (headArray->next != NULL) {
		headArray = headArray->next;
	}
	headArray->next = newElement;
	return true;
}

void P_ClearLinkedArray(LinkedArray* headArray) {
	while (headArray != NULL) {
		free(headArray->array);
		LinkedArray* temp = headArray;
		headArray = headArray->next;
		free(temp);
	}
	headArray = NULL;
}

// TODO: Schiften zu neueren File?

byte P_IsRegister(char* input) {
	P_ToLower(input);
	if (P_StrCmp(input, "r0")) {
		return 0;
	}
	else if (P_StrCmp(input, "r1")) {
		return 1;
	}
	else if (P_StrCmp(input, "r2")) {
		return 2;
	}
	else if (P_StrCmp(input, "r3")) {
		return 3;
	}
	else if (P_StrCmp(input, "r4")) {
		return 4;
	}
	else if (P_StrCmp(input, "r5")) {
		return 5;
	}
	else if (P_StrCmp(input, "r6")) {
		return 6;
	}
	else if (P_StrCmp(input, "r7")) {
		return 7;
	}
	else if (P_StrCmp(input, "r8")) {
		return 8;
	}
	else if (P_StrCmp(input, "r9")) {
		return 9;
	}
	else if (P_StrCmp(input, "ir0")) {
		return 10;
	}
	else if (P_StrCmp(input, "ir1")) {
		return 11;
	}
	else if (P_StrCmp(input, "ir2")) {
		return 12;
	}
	else if (P_StrCmp(input, "ir3")) {
		return 13;
	}
	else if (P_StrCmp(input, "ir4")) {
		return 14;
	}
	else if (P_StrCmp(input, "ir5")) {
		return 15;
	}
	else if (P_StrCmp(input, "sr0")) {
		return 0;
	}
	else if (P_StrCmp(input, "sr1")) {
		return 1;
	}
	else if (P_StrCmp(input, "sr2")) {
		return 2;
	}
	else if (P_StrCmp(input, "sr3")) {
		return 3;
	}
	return 255;
}

unsigned short P_ValidateOpCode(char *input) {
	P_ToLower(input);
	if (P_StrCmp(input, "set")) {
		return set;
	} else if (P_StrCmp(input, "inc")) {
		return inc;
	} else if (P_StrCmp(input, "dec")) {
		return dec;
	} else if (P_StrCmp(input, "neg")) {
		return neg;
	} else if (P_StrCmp(input, "cmp")) {
		return cmp;
	} else if (P_StrCmp(input, "mov")) {
		return mov;
	} else if (P_StrCmp(input, "swi")) {
		return swi;
	} else if (P_StrCmp(input, "clr")) {
		return clr;
	} else if (P_StrCmp(input, "signed")) {
		return signedFlag;
	} else if (P_StrCmp(input, "load")) {
		return load;
	} else if (P_StrCmp(input, "store")) {
		return store;
	} else if (P_StrCmp(input, "loadb")) {
		return loadB;
	} else if (P_StrCmp(input, "storeb")) {
		return storeB;
	} else if (P_StrCmp(input, "rswitch")) {
		return rswitch;
	} else if (P_StrCmp(input, "add")) {
		return add;
	} else if (P_StrCmp(input, "sub")) {
		return sub;
	} else if (P_StrCmp(input, "mul")) {
		return mul;
	} else if (P_StrCmp(input, "div")) {
		return division;
	} else if (P_StrCmp(input, "mod")) {
		return mod;
	} else if (P_StrCmp(input, "gethigh")) {
		return getHigh;
	} else if (P_StrCmp(input, "getlow")) {
		return getLow;
	} else if (P_StrCmp(input, "rmc")) {
		return rmc;
	} else if (P_StrCmp(input, "and")) {
		return and;
	} else if (P_StrCmp(input, "or")) {
		return or;
	} else if (P_StrCmp(input, "xor")) {
		return xor;
	} else if (P_StrCmp(input, "not")) {
		return not;
	} else if (P_StrCmp(input, "shl")) {
		return shl;
	} else if (P_StrCmp(input, "shr")) {
		return shr;
	} else if (P_StrCmp(input, "jmp")) {
		return jmp;
	} else if (P_StrCmp(input, "call")) {
		return call;
	} else if (P_StrCmp(input, "je")) {
		return je;
	} else if (P_StrCmp(input, "jne")) {
		return jne;
	} else if (P_StrCmp(input, "jz")) {
		return jz;
	} else if (P_StrCmp(input, "jnz")) {
		return jnz;
	} else if (P_StrCmp(input, "jg")) {
		return jg;
	} else if (P_StrCmp(input, "jge")) {
		return jge;
	} else if (P_StrCmp(input, "jl")) {
		return jl;
	} else if (P_StrCmp(input, "jle")) {
		return jle;
	} else if (P_StrCmp(input, "jc")) {
		return jc;
	} else if (P_StrCmp(input, "jnc")) {
		return jnc;
	} else if (P_StrCmp(input, "jn")) {
		return jNegativ;
	} else if (P_StrCmp(input, "jnn")) {
		return jnn;
	} else if (P_StrCmp(input, "ret")) {
		return ret;
	} else if (P_StrCmp(input, "hop")) {
		return hop;
	} else if (P_StrCmp(input, "push")) {
		return push;
	} else if (P_StrCmp(input, "pop")) {
		return pop;
	} else if (P_StrCmp(input, "peek")) {
		return peek;
	} else if (P_StrCmp(input, "pushb")) {
		return pushB;
	} else if (P_StrCmp(input, "popb")) {
		return popB;
	} else if (P_StrCmp(input, "peekb")) {
		return peekB;
	} else if (P_StrCmp(input, "syscall")) {
		return syscall;
	} else if (P_StrCmp(input, "nop")) {
		return nop;
	} else if (P_StrCmp(input, "halt")) {
		return halt;
	} else if (P_StrCmp(input, "setmmu")) {
		return setMMU;
	} else if (P_StrCmp(input, "setstkptr")) {
		return setStkPtr;
	} else if (P_StrCmp(input, "setstkadress")) {
		return setStkAdress;
	} else if (P_StrCmp(input, "getsreg")) {
		return getSReg;
	} else if (P_StrCmp(input, "setpc")) {
		return setPC;
	} else if (P_StrCmp(input, "setadressspace")) {
		return setAdressSpace;
	} else if (P_StrCmp(input, "setclock")) {
		return setClock;
	} else if (P_StrCmp(input, "getclock")) {
		return getClock;
	} else if (P_StrCmp(input, "sysret")) {
		return sysRet;
	} else if (P_StrCmp(input, "getregs")) {
		return getRegs;
	} else if (P_StrCmp(input, "setregs")) {
		return setRegs;
	} else if (P_StrCmp(input, "exec")) {
		return exec;
	} else if (P_StrCmp(input, "setpx")) {
		return setPx;
	} else if (P_StrCmp(input, "startframe")) {
		return startFrame;
	} else if (P_StrCmp(input, "addint")) {
		return addInt;
	} else if (P_StrCmp(input, "rmvint")) {
		return rmvInt;
	} else if (P_StrCmp(input, "getint")) {
		return getInt;
	} else if (P_StrCmp(input, "di")) {
		return di;
	} else if (P_StrCmp(input, "ei")) {
		return ei;
	} else if (P_StrCmp(input, "retint")) {
		return retInt;
	} else if (P_StrCmp(input, "loadblock")) {
		return loadBlock;
	} else if (P_StrCmp(input, "writeblock")) {
		return writeBlock;
	} else if (P_StrCmp(input, "restart")) {
		return restart;
	} else if (P_StrCmp(input, "exit")) {
		return terminate;
	}
	return notDefined;
}

