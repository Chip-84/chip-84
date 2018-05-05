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

gfx_UninitedSprite(scaled_sprite, 255, 128);

ti_var_t curFile;

char files[255][9];
uint8_t count;

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
	
	boot_Set48MHzMode();
	
	count = 0;
	
	ti_CloseAll();
	
	gfx_Begin(gfx_8bpp);
	gfx_Blit(gfx_screen);
    gfx_SetDrawBuffer();
	gfx_SetPalette(sprites_gfx_pal, sizeof_sprites_gfx_pal, 0);
	gfx_SetColor(0xff);
	
	gfx_SetTextFGColor(0x00);
	gfx_PrintStringXY("Chip-84", 128, 100);
	gfx_PrintStringXY("2018 Christian Kosman", 80, 120);
	gfx_PrintStringXY("version 1.0", LCD_WIDTH-100, LCD_HEIGHT-30);
	gfx_BlitBuffer();
	
	delay(1000);
	
	while((varName = ti_Detect(&search_pos, "Chip84")) != NULL) {
		dbg_sprintf(dbgout, "%s\n", varName);
		
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
			while(kb_Data[7] & kb_Up) {
				kb_Scan();
			}
		} else if(kb_Data[7] & kb_Down) {
			if(startPos == count-1)
				startPos = 0;
			else
				startPos++;
			drawMenu(startPos);
			while(kb_Data[7] & kb_Down) {
				kb_Scan();
			}
		} else if(kb_Data[6] & kb_Enter) {
			gfx_FillScreen(0xFF);
			startEmulation(files[startPos]);
			break;
		}
		
		gfx_BlitBuffer();
	} while (kb_Data[1] != kb_2nd);
	
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
	
	gfx_PrintStringXY("Use the arrows and enter to select a ROM.", 20, 190);
	gfx_PrintStringXY("Then use 1, 2, 3, PLUS, 4, 5, 6, MINUS, 7, 8, 9,", 20, 205);
	gfx_PrintStringXY("TIMES, COMMA, (, ), and DIVIDE for controls.", 20, 215);
	
}

void startEmulation(char *fileName) {
	scaled_sprite->width = 255;
	scaled_sprite->height = 128;
	
	loadProgram(fileName);
	
	gfx_FillScreen(0x88);
	gfx_SetTextFGColor(0x01);
	gfx_PrintStringXY("[2nd] - Quit", 10, LCD_HEIGHT-20);
	
	do {
		
		emulateCycle(10);
		
		if(drawFlag)
			drawGraphics();
		
		gfx_BlitBuffer();
	} while (kb_Data[1] != kb_2nd);
}

void drawGraphics() {
	drawFlag = false;
	
	gfx_ScaleSprite(canvas, scaled_sprite);
	gfx_Sprite(scaled_sprite, 30, 55);
}