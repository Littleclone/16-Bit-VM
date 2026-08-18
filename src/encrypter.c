#include "../header/encrypter.h"
#include <stdio.h>
#include <stdlib.h>

// Erstellt ein key in dem es in die H�lfte der File geht, dann die ersten 10 Bytes (_Signed) als Offsets nimmt und
// dann den 11 Byte als ersten Teil des Schl�ssels, dies wird 10 Mal gemacht damit man am ende ein Schl�ssel aus 10 Bytes
// hat. Beim wiederholen wird von der H�lfte der file um 1 Incrementiert.
// Damit alles sicher funktioniert wird der Anfang der File und das Ende genommen und dann wird immer geschaut bevor
// der Offset übertragen wird ob es dann am ende w�re, wenn ja wird es geteilt durch 2 Genommen.
// Dieser Key wird dann am ende rangeh�ngt (Eine File ohne Key ist ung�ltig)
unsigned char* GetEncryptionKey(FILE* file) { // TODO: Nochmal �berpr�fen
    unsigned char* _key = (unsigned char*)malloc(11);
    if (_key == NULL) {
        perror("Key konnte nicht allokiert werden\n");
        return NULL;
    }
    long _endOfFile = 0;
    fseek(file, 0, SEEK_END);
    _endOfFile = ftell(file);
    long _halfOfFile = _endOfFile / 2;
    long _tempHalfOfFile = _halfOfFile; // TODO: Vielleicht nicht n�tig
    fseek(file, _halfOfFile, SEEK_SET);
    for (byte bytes = 1; bytes <= 10; ++bytes) {
        for (byte counter = 0; counter < 10; ++counter) {
            sbyte _offset = fgetc(file);
            while (true) {
                if ((_tempHalfOfFile + _offset) < _endOfFile && (_tempHalfOfFile + _offset) > 0) {
                    fseek(file, _offset, SEEK_CUR);
                }
                else {
                    _offset /= 2;
                    _tempHalfOfFile = _halfOfFile;
                    continue;
                }
            }
        }
        _key[bytes] = fgetc(file);
        _tempHalfOfFile = (_halfOfFile + bytes);
        fseek(file, _tempHalfOfFile, SEEK_SET);
    }
    _key[10] = '\0';
    return _key;
}