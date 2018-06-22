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
void drawPreview(uint8_t x, uint8_t y);

ti_var_t curFile;

char files[255][9];
uint8_t count;

uint8_t bgColor = 0x88;
uint8_t cpf = 10;

uint8_t frameCount = 0;

const char *pauseText = "[2nd] - Pause";
const char *keyNames[16] = {
	"Decimal",
	"Seven",
	"Eight",
	"Nine",
	"Four",
	"Five",
	"Six",
	"One",
	"Two",
	"Three",
	"Zero",
	"Negative",
	"Times",
	"Plus",
	"Minus",
	"Enter"
};

void main(void) {
	uint8_t i;
	char *varName;
	uint8_t *search_pos = NULL;
	uint8_t startPos = 0;
	uint8_t test1[16] = {0x00,0x01,0x02,0x03,0x06,0x05,0x04,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
	
	boot_Set48MHzMode();
	
	count = 0;
	
	gfx_Begin(gfx_8bpp);
	gfx_Blit(gfx_screen);
    gfx_SetDrawBuffer();
	gfx_SetPalette(sprites_gfx_pal, sizeof_sprites_gfx_pal, 0);
	gfx_SetColor(0xff);
	
	gfx_SetTextFGColor(0x00);
	gfx_SetTextScale(2, 2);
	gfx_PrintStringXY("Chip-84", 103, 95);
	gfx_SetTextScale(1, 1);
	gfx_PrintStringXY("2018 Christian Kosman", 80, 120);
	gfx_PrintStringXY("version 2.2.0", LCD_WIDTH-100, LCD_HEIGHT-30);
	gfx_BlitBuffer();
	
	delay(1000);
	
	while((varName = ti_Detect(&search_pos, "Chip84")) != NULL) {
		strcpy(files[count], varName);
		++count;
	}
	
	drawMenu(0);
	
	do {
		kb_Scan();
		
		if(kb_Data[7] & kb_Up) {
			frameCount = 0;
			if(startPos == 0)
				startPos = count-1;
			else
				startPos--;
			drawMenu(startPos);
		} else if(kb_Data[7] & kb_Down) {
			frameCount = 0;
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
		
		if(frameCount > 9) {
			if(frameCount < 100) {
				frameCount = 101;
				loadProgram(files[startPos]);
				gfx_PrintStringXY("Preview", 187, 60);
			}
			emulateCycle(20);
			
			if(drawFlag)
				drawPreview(150, 70);
		} else {
			frameCount++;
			delay(100);
		}
		
		gfx_BlitBuffer();
	} while (kb_Data[6] != kb_Clear);
	
	gfx_End();
}

void drawMenu(uint8_t start) {
	uint8_t i;
	gfx_FillScreen(0xff);
	gfx_SetTextFGColor(0x00);
	for(i = 0; i < 16; i++) {
		if(i + start <= count-1) {
			//gfx_SetTextXY(20, 10*i+50);
			//gfx_PrintUInt(i+start, 3);
			gfx_PrintStringXY(files[i+start], 30, 10*i+50);
		}
	}
	gfx_PrintStringXY(">",20,50);
	
	//gfx_PrintStringXY("Use the arrows and enter to select a ROM.", 10, 180);
	//gfx_PrintStringXY("Then use 1, 2, 3, PLUS, 4, 5, 6, MINUS, 7, 8, 9,", 10, 195);
	//gfx_PrintStringXY("TIMES, 0, DECIMAL, ( - ), and ENTER to control.", 10, 205);
	gfx_SetTextScale(2,2);
	gfx_PrintStringXY("Select a ROM", 20, 20);
	gfx_SetTextScale(1,1);
	gfx_PrintStringXY("[clear] - Quit", 220, 220);
	
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
			uint8_t selected_option = 0;
			
			gfx_FillScreen(bgColor);
			drawPreview(170,135);
			gfx_SetTextScale(2, 2);
			gfx_PrintStringXY("Paused", 20, 20);
			gfx_SetTextScale(1, 1);
			gfx_PrintStringXY("Keymapping", 20, 50);
			gfx_PrintStringXY("Cycles per frame -", 20, 60);
			gfx_PrintStringXY("Resume", 20, 70);
			gfx_PrintStringXY("Quit game", 20, 80);
			gfx_BlitBuffer();
			gfx_SetColor(bgColor);
			while(paused) {
				kb_Scan();
				
				gfx_FillRectangle(10, 50 + selected_option*10, 10, 10);
				
				if(kb_Data[7] & kb_Up) {
					selected_option--;
					if(selected_option == 255) selected_option = 3;
				}
				if(kb_Data[7] & kb_Down) {
					selected_option++;
					if(selected_option == 4) selected_option = 0;
				}
				
				gfx_PrintStringXY(">", 10, 50 + selected_option*10);
				
				if(selected_option == 0) {
					if(kb_Data[6] & kb_Enter) {
						beginKeymapper(fileName);
						gfx_PrintStringXY(pauseText, 10, LCD_HEIGHT-20);
					}
				}
				
				if(selected_option == 1) {
					if(kb_Data[7] & kb_Left) {
						cpf--;
					}
					if(kb_Data[7] & kb_Right) {
						cpf++;
					}
				}
				gfx_FillRectangle(150, 60, 30, 30);
				gfx_SetTextXY(150, 60);
				gfx_PrintUInt(cpf, 2);
				
				if(kb_Data[6] & kb_Clear || (selected_option == 3 && kb_Data[6] & kb_Enter)) {
					while(kb_Data[6] & kb_Clear || kb_Data[6] & kb_Enter) {
						kb_ScanGroup(6);
					}
					playing = false;
					break;
				}
				if(kb_Data[1] & kb_2nd || (selected_option == 2 && kb_Data[6] & kb_Enter)) {
					while(kb_Data[1] & kb_2nd || kb_Data[6] & kb_Enter) {
						kb_Scan();
					}
					paused = false;
					gfx_SetColor(bgColor);
					gfx_FillScreen(bgColor);
					drawGraphics();
					gfx_PrintStringXY(pauseText, 10, LCD_HEIGHT-20);
				}
				
				gfx_BlitBuffer();
				delay(100);
			}
		}
	} while (playing);
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
			if(kb_Data[1] & kb_2nd) {
				for(i = 0; i < 16; i++) {
					game_data[i+6] = i;
					controlMap[i] = i;
				}
				drawKeymappingMenu(selected);
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
	} while(kb_Data[6] != kb_Clear);
	
	while(kb_Data[6] & kb_Clear) {
		kb_ScanGroup(6);
	}
	
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
		gfx_PrintStringXY("Key", 30, i*10+10);
		gfx_SetTextXY(60, i*10+10);
		gfx_PrintUInt(i, 2);
		gfx_PrintStringXY(keyNames[game_data[i+6]], 90, i*10+10);
	}
	
	gfx_PrintStringXY("Select a key to remap, then press the new key.", 10, 200);
	gfx_PrintStringXY("Press 2nd to reset, clear to exit.", 10, 215);
	
	gfx_SetColor(0x01);
	if(selected > 16) {
		gfx_Circle(13, (selected%16)*10+13, 3);
	} else {
		gfx_FillRectangle(10, (selected%16)*10+10, 6, 6);
	}
	
	gfx_BlitBuffer();
}

void drawGraphics() {
	drawFlag = false;
	
	if(extendedScreen)
		gfx_ScaledSprite_NoClip(scanvas, 30, 55, 2, 2);
	else
		gfx_ScaledSprite_NoClip(canvas, 30, 55, 4, 4);
}

void drawPreview(uint8_t x, uint8_t y) {
	drawFlag = false;
	
	if(extendedScreen)
		gfx_Sprite_NoClip(scanvas, x, y);
	else
		gfx_ScaledSprite_NoClip(canvas, x, y, 2, 2);
}