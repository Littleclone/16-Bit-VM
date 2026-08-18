#ifndef chr_ROM_h
#define chr_ROM_h

#include "essentials.h"

#define sprite_size (16 * 16)

struct frame {
	int RGBA[sprite_size]; // Das Array mit den Pixeln
};

// Diese hier wird am anfang benötigt wenn die Anzahl der Frames noch unklar ist.
struct frame_list {
	struct frame* frame;
	struct frame_list* next;
	unsigned short id;
	bool hasAnimation;
	unsigned int animationFrames;
};

// Ist die Liste die die Frame_List hält
struct list {
	struct frame_list* sprite_Frames;
	struct list* next;
};

struct sprite {
	struct frame* spriteArray;
	bool hasAnimation;
	unsigned short id;
	unsigned int current_Frame;
};

bool initCHR_ROM();
void FreeCHR_ROM();

void LoadCHR_Sprite(const unsigned short _sprite_pos_x, const unsigned short _sprite_pos_y, const unsigned short _sprite_id);

#endif // !chr_ROM_h

