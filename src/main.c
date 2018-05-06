#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>
#include <debug.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <graphx.h>
#include <keypadc.h>
#include <fileioc.h>

#include "chip8.h"
#include "sprites_gfx.h"

void drawGraphics(void);
void startEmulation(char *fileName);
void drawMenu(uint8_t start);
void drawKeymappingMenu(uint8_t selected);
void beginKeymapper(char *fileName);
void beginSetClock(void);

ti_var_t curFile;

char files[255][9];
uint8_t count;

uint8_t bgColor = 0x88;
uint8_t cpf = 10;

const char *pauseText = "[2nd] - Pause";

uint8_t concatenate(uint8_t x, uint8_t y) {
	uint8_t pow = 10;
	while (y >= pow)
		pow *= 10;
	return x * pow + y;
}

void main(void) {
	uint8_t i;
	char *varName;
	uint8_t *search_pos = NULL;
	uint8_t startPos = 0;
	uint8_t test1[16] = {0x00,0x01,0x02,0x03,0x06,0x05,0x04,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
	
	boot_Set48MHzMode();
	
	count = 0;
	
	//ti_CloseAll();
	//curFile = ti_Open("C84Cfg", "a");
	//ti_Write(&test1, 16, 1, curFile);
	
	gfx_Begin(gfx_8bpp);
	gfx_Blit(gfx_screen);
    gfx_SetDrawBuffer();
	gfx_SetPalette(sprites_gfx_pal, sizeof_sprites_gfx_pal, 0);
	gfx_SetColor(0xff);
	
	gfx_SetTextFGColor(0x00);
	gfx_PrintStringXY("Chip-84", 128, 100);
	gfx_PrintStringXY("2018 Christian Kosman", 80, 120);
	gfx_PrintStringXY("version 1.1", LCD_WIDTH-100, LCD_HEIGHT-30);
	gfx_BlitBuffer();
	
	delay(1000);
	
	while((varName = ti_Detect(&search_pos, "Chip84")) != NULL) {
		//dbg_sprintf(dbgout, "%s\n", varName);
		
		strcpy(files[count], varName);
		
		++count;
	}
	
	drawMenu(0);
	
	do {
		kb_Scan();
		
		if(kb_Data[7] & kb_Up) {
			if(startPos == 0)
				startPos = count-1;
			else
				startPos--;
			drawMenu(startPos);
		} else if(kb_Data[7] & kb_Down) {
			if(startPos == count-1)
				startPos = 0;
			else
				startPos++;
			drawMenu(startPos);
		} else if(kb_Data[6] & kb_Enter) {
			gfx_FillScreen(0xFF);
			startEmulation(files[startPos]);
			drawMenu(startPos);
		}
		
		gfx_BlitBuffer();
		delay(100);
	} while (kb_Data[6] != kb_Clear);
	
	gfx_End();
}

void drawMenu(uint8_t start) {
	uint8_t i;
	gfx_FillScreen(0xff);
	gfx_SetTextFGColor(0x00);
	for(i = 0; i < 16; i++) {
		if(i + start <= count-1) {
			gfx_SetTextXY(20, 10*i+10);
			gfx_PrintUInt(i+start, 3);
			gfx_PrintStringXY(files[i+start], 50, 10*i+10);
		}
	}
	gfx_SetColor(0x00);
	gfx_FillRectangle(10, 10, 6, 6);
	
	gfx_PrintStringXY("Use the arrows and enter to select a ROM.", 20, 180);
	gfx_PrintStringXY("Then use 1, 2, 3, PLUS, 4, 5, 6, MINUS, 7, 8, 9,", 20, 195);
	gfx_PrintStringXY("TIMES, COMMA, (, ), and DIVIDE for controls.", 20, 205);
	gfx_PrintStringXY("Press clear to quit.", 20, 220);
	
}

void startEmulation(char *fileName) {
	
	loadProgram(fileName);
	
	gfx_FillScreen(bgColor);
	gfx_SetTextFGColor(0x01);
	gfx_PrintStringXY(pauseText, 10, LCD_HEIGHT-20);
	
	do {
		emulateCycle(cpf);
		
		if(drawFlag)
			drawGraphics();
		
		gfx_BlitBuffer();
		if(paused) {
			gfx_PrintStringXY("Paused", 10, 10);
			gfx_PrintStringXY("[clear] - Exit", 10, 20);
			gfx_PrintStringXY("[alpha] - Keymapping", 10, 30);
			gfx_PrintStringXY("[mode] - CPF", 10, 40);
			gfx_BlitBuffer();
			while(paused) {
				kb_Scan();
				if(kb_Data[6] & kb_Clear) {
					while(kb_Data[6] & kb_Clear) {
						kb_ScanGroup(6);
					}
					playing = false;
					break;
				}
				if(kb_Data[1] & kb_2nd) {
					while(kb_Data[1] & kb_2nd) {
						kb_Scan();
					}
					paused = false;
					gfx_SetColor(bgColor);
					gfx_FillRectangle(10, 10, 100, 50);
				}
				if(kb_Data[2] & kb_Alpha) {
					while(kb_Data[2] & kb_Alpha) {
						kb_Scan();
					}
					beginKeymapper(fileName);
					gfx_PrintStringXY(pauseText, 10, LCD_HEIGHT-20);
				}
				if(kb_Data[1] & kb_Mode) {
					while(kb_Data[1] & kb_Mode) {
						kb_Scan();
					}
					beginSetClock();
					gfx_PrintStringXY(pauseText, 10, LCD_HEIGHT-20);
				}
			}
		}
	} while (playing);
}

void beginSetClock() {
	gfx_FillScreen(bgColor);
	gfx_PrintStringXY("Set the amount of cycles emulated per frame.", 10, 10);
	gfx_PrintStringXY("Press mode to exit.", 10, 25);
	
	gfx_SetTextXY(140, 115);
	gfx_PrintUInt(cpf, 2);
	gfx_SetColor(bgColor);
	
	gfx_BlitBuffer();
	do {
		kb_Scan();
		if(kb_Data[7] & kb_Down) {
			cpf--;
			if(cpf == 0) cpf = 1;
			gfx_FillRectangle(140, 115, 30, 30);
			gfx_SetTextXY(140, 115);
			gfx_PrintUInt(cpf, 2);
			gfx_BlitBuffer();
		}
		if(kb_Data[7] & kb_Up) {
			cpf++;
			if(cpf > 10) cpf = 10;
			gfx_FillRectangle(140, 115, 30, 30);
			gfx_SetTextXY(140, 115);
			gfx_PrintUInt(cpf, 2);
			gfx_BlitBuffer();
		}
		delay(100);
	} while(kb_Data[1] != kb_Mode);
	gfx_FillScreen(bgColor);
	gfx_BlitBuffer();
	paused = false;
}

void beginKeymapper(char *fileName) {
	uint8_t selected = 0;
	uint8_t i;
	bool awaiting = false;
	
	drawKeymappingMenu(selected);
	
	do {
		kb_Scan();
		if(!awaiting) {
			if(kb_Data[7] & kb_Down) {
				selected++;
				if(selected > 16) selected = 0;
				drawKeymappingMenu(selected);
			}
			if(kb_Data[7] & kb_Up) {
				if(selected == 0)
					selected = 16;
				else
					selected--;
				drawKeymappingMenu(selected);
			}
			if(kb_Data[6] & kb_Enter) {
				drawKeymappingMenu(selected+16);
				awaiting = 1;
			}
			delay(100);
		} else {
			keypad[0x0] = kb_Data[4] & kb_DecPnt;
			keypad[0x1] = kb_Data[3] & kb_7;
			keypad[0x2] = kb_Data[4] & kb_8;
			keypad[0x3] = kb_Data[5] & kb_9;
			keypad[0x4] = kb_Data[3] & kb_4;
			keypad[0x5] = kb_Data[4] & kb_5;
			keypad[0x6] = kb_Data[5] & kb_6;
			keypad[0x7] = kb_Data[3] & kb_1;
			keypad[0x8] = kb_Data[4] & kb_2;
			keypad[0x9] = kb_Data[5] & kb_3;
			keypad[0xA] = kb_Data[3] & kb_0;
			keypad[0xB] = kb_Data[5] & kb_Chs;
			keypad[0xC] = kb_Data[6] & kb_Mul;
			keypad[0xD] = kb_Data[6] & kb_Sub;
			keypad[0xE] = kb_Data[6] & kb_Add;
			keypad[0xF] = kb_Data[6] & kb_Enter;
			
			for(i = 0; i < 16; i++) {
				if(keypad[i]) {
					game_data[selected+6] = i;
					controlMap[selected] = i;
					awaiting = 0;
					drawKeymappingMenu(selected);
				}
			}
		}
	} while(kb_Data[2] != kb_Alpha);
	
	ti_CloseAll();
	curFile = ti_Open(fileName, "w");
	ti_Write(&game_data, sizeof(game_data)/sizeof(uint8_t), 1, curFile);
	
	gfx_FillScreen(bgColor);
	gfx_BlitBuffer();
	paused = false;
}

void drawKeymappingMenu(uint8_t selected) {
	uint8_t i;
	gfx_FillScreen(bgColor);
	
	for(i = 0; i < 16; i++) {
		gfx_PrintStringXY("Key", 10, i*10+10);
		gfx_SetTextXY(40, i*10+10);
		gfx_PrintUInt(i, 2);
		gfx_SetTextXY(70, i*10+10);
		gfx_PrintUInt(game_data[i+6], 2);
	}
	
	gfx_PrintStringXY("Select a key to remap, then press the new key.", 10, 200);
	gfx_PrintStringXY("Press alpha to exit.", 10, 215);
	
	gfx_SetColor(0x01);
	if(selected > 16) {
		gfx_Circle(103, (selected%16)*10+13, 3);
	} else {
		gfx_FillRectangle(100, (selected%16)*10+10, 6, 6);
	}
	
	gfx_BlitBuffer();
}

void drawGraphics() {
	drawFlag = false;
	
	gfx_ScaledSprite_NoClip(canvas, 30, 55, 4, 4);
}