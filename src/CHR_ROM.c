#include "../header/CHR_ROM.h"
#include <stdio.h>
#include <stdlib.h>
#include "../header/bunnySystem.h"
#include "../header/GPU.h"
#include "../header/VRAM.h"

char* ROM_Path = NULL;
struct sprite* Sprite_head = NULL;
unsigned short Sprites = 0;

bool L_SetupSprites(struct list** list_Head_ptr, unsigned short sprite_array);

// TODO: Müsste nachschauen ob das überhaupt Funktioniert.
bool initCHR_ROM() {
	if (ROM_Path == NULL) {
#if PLATFORM_WINDOWS
		ROM_Path = P_StrAdd(P_KernelPath, "\\CHR_ROM.bin");
#elif PLATFORM_LINUX
		ROM_Path = P_StrAdd(G_KernelPath, "/CHR_ROM.bin");
#endif
		if (ROM_Path == NULL) {
			log("CHR_ROM konnte nicht erstellt werden.")
			return false;
		}
	}

	unsigned short sprite_counter = 0;
	unsigned int frame_counter = 1;
	FILE* CHR_ROM_file = fopen(ROM_Path, "rb");
	if (CHR_ROM_file == NULL) {
		printf("Error: CHR_ROM [1]\n");
		return false;
	}
	struct frame_list** frame_head_ptr = NULL;
	struct list** list_Head_ptr = NULL;
	while (feof(CHR_ROM_file)) {
		struct frame_list* new_Entry = (struct frame_list*)malloc(sizeof(struct frame_list));
		if (new_Entry == NULL) {
			printf("Error: CHR_ROM [2]\n");
			return false;
		}
		new_Entry->next = NULL;
		byte flags = fgetc(CHR_ROM_file);
		new_Entry->hasAnimation = flags & 1;
		unsigned short id = fgetc(CHR_ROM_file) << 8;
		id |= fgetc(CHR_ROM_file);
		new_Entry->id = id;
		struct frame* frame = (struct frame*)malloc(sizeof(struct frame));
		if (frame == NULL) {
			printf("Error: CHR_ROM [3]\n");
			return false;
		}
		// TODO: Theoretisch m�sste "byte" reichen weil 16*16 = 256 ist.
		for (unsigned short _pixel_counter = 0; _pixel_counter < sprite_size; ++_pixel_counter) {
			frame->RGBA[_pixel_counter] = fgetc(CHR_ROM_file) << 24;
			frame->RGBA[_pixel_counter] |= fgetc(CHR_ROM_file) << 16;
			frame->RGBA[_pixel_counter] |= fgetc(CHR_ROM_file) << 8;
			frame->RGBA[_pixel_counter] |= fgetc(CHR_ROM_file);
		}
		// Eintragung in die Linked List
		new_Entry->frame = frame;
		if (frame_head_ptr != NULL) {
			struct frame_list* _temp = *frame_head_ptr;
			while (_temp->next != NULL) {
				_temp = _temp->next;
			}
			_temp->next = new_Entry;
		}
		else {
			frame_head_ptr = &new_Entry;
		}
		unsigned char _flag = fgetc(CHR_ROM_file);
		if (_flag == '?') {
			struct frame_list* _head = *frame_head_ptr;
			_head->animationFrames = frame_counter;
			frame_counter = 1;
			frame_head_ptr = NULL;
			struct list* _entry = (struct list*)malloc(sizeof(struct list));
			if (_entry == NULL) {
				printf("Error: CHR_ROM [4]\n");
				return false;
			}
			_entry->sprite_Frames = _head;
			if (list_Head_ptr != NULL) {
				struct list* _temp = *list_Head_ptr;
				while (_temp->next != NULL) {
					_temp = _temp->next;
				}
				_temp->next = _entry;
			}
			else {
				list_Head_ptr = &_entry;
			}
			++sprite_counter;
		}
		else if (_flag == '!') {
			++frame_counter;
		}
		else {
			printf("Error: CHR_ROM [5]\n");
			return false;
		}
	}
	fclose(CHR_ROM_file);
	bool _result = L_SetupSprites(list_Head_ptr, sprite_counter);
	return _result;
}

bool L_SetupSprites(struct list** list_Head_ptr, unsigned short sprite_array) {
	Sprites = sprite_array;
	struct sprite* _sprite_array_ptr = (struct sprite*)malloc(sprite_array * (sizeof(struct sprite)));
	struct list* _frame_head = *list_Head_ptr;
	if (_sprite_array_ptr == NULL) {
		printf("Error: CHR_ROM [6]\n");
		return false;
	}
	Sprite_head = _sprite_array_ptr;
	unsigned int _counter = 0;
	while (_frame_head != NULL && _counter != sprite_array) {
		struct frame_list* _temp_head = _frame_head->sprite_Frames;
		struct list* _temp_list = _frame_head;
		unsigned int _frames = _frame_head->sprite_Frames->animationFrames; // TODO: Idk wieso es sich beschwert hat bei temp_head mit speicher
		if (_frames <= 1) {
			_sprite_array_ptr[_counter].id = _temp_head->id;
			_sprite_array_ptr[_counter].hasAnimation = 0;
			_sprite_array_ptr[_counter].spriteArray = _temp_head->frame;
			_sprite_array_ptr[_counter].current_Frame = 0;
			_frame_head = _frame_head->next;
		}
		else {
			_sprite_array_ptr[_counter].id = _temp_head->id;
			_sprite_array_ptr[_counter].hasAnimation = _temp_head->hasAnimation;
			_sprite_array_ptr[_counter].current_Frame = 0;
			_sprite_array_ptr[_counter].spriteArray = (struct frame*)malloc(_frames * (sizeof(struct frame)));
			if (_sprite_array_ptr[_counter].spriteArray == NULL) {
				printf("Error: CHR_ROM [7]\n");
				return false;
			}
			for (unsigned short i = 0; i < _frames; ++i) {
				for (unsigned short j = 0; j < sprite_size; ++j) {
					_sprite_array_ptr[_counter].spriteArray[i].RGBA[j] = _temp_head->frame->RGBA[j];
				}
				struct frame_list* _temp2 = _temp_head;
				_temp_head = _temp_head->next;
				free(_temp2->frame->RGBA);
				free(_temp2->frame);
				free(_temp2);
			}
		}
		_frame_head = _frame_head->next;
		if (_temp_head != NULL) { // TODO: Mal schauen wie gut das Klappt (Memory Leak Potential)
			free(_temp_head->frame->RGBA);
			free(_temp_head->frame);
			free(_temp_head);
		}
		free(_temp_list);
		++_counter;
	}
	return true;
}

void FreeCHR_ROM() {
	free(ROM_Path);
	free(Sprite_head);
	ROM_Path = NULL;
	Sprite_head = NULL;
}

// Wie soll die Struktur des CHR_ROM sein?
/*
* 1 Bit ob es ein Sprite mit Animationen ist
* 1 Bit Ist AutoPlay [Removed?]
* 6 Bits Unused?
* 2 Bytes f�r die ID
* Ein Frame besteht aus 16x16 Pixel, ein Pixel ist 4 Bytes gro� ---> RGBA
* ! <--- Sagt das es mehr Frames gibt (Wird erwartet)
* ? <--- Sagt das dies das ende dieses Sprites ist.
*/

// TODO: Testen
void LoadCHR_Sprite(const unsigned short _sprite_pos_x, const unsigned short _sprite_pos_y, const unsigned short _sprite_id) {
	if (_sprite_id > Sprites) {
		printf("Error: Sprite ID ist �ber CHR-Rom size\n");
		return;
	}
	struct sprite _CurrentSprite = Sprite_head[_sprite_id];
	unsigned int _posX = (unsigned int)_sprite_pos_x, _posY = (unsigned int)_sprite_pos_y;
	// Derzeit f�r Tests
#if IsDev
	if (_CurrentSprite.id != _sprite_id) {
		printf("Error: Sprite ID ist nicht gew�nschte ID\n");
	}
#endif
	unsigned short _pixel_id = 0;
	for (byte y_modifier = 1; 0 < Sprite_Height; ++y_modifier) {
		_posY = _sprite_pos_y * y_modifier;
		for (byte x_modifier = 0; 0 < Sprite_Width; ++x_modifier) {
			_posX = _sprite_pos_x + x_modifier;
			_posX += _posY * ScreenWidth;
			//StoreVRAM64(_posX, _CurrentSprite.spriteArray->RGBA[_pixel_id]);
			++_pixel_id;
		}
	}
}//
// Created by hannah on 24/06/25.
//
