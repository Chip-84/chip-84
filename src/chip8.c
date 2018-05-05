#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>
#include <debug.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fileioc.h>
#include <keypadc.h>

#include "chip8.h"
#include "sprites_gfx.h"

uint16_t opcode = 0;
uint8_t memory[4096];
uint8_t V[16];
uint16_t I = 0;
uint16_t pc = 0;
int16_t delay_timer = 0;
int16_t sound_timer = 0;
uint16_t stack[16];
uint8_t sp = 0;
uint8_t keys[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
bool drawFlag = false;

uint8_t game_data[3584];

ti_var_t file;

unsigned char fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, //0
    0x20, 0x60, 0x20, 0x20, 0x70, //1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
    0x90, 0x90, 0xF0, 0x10, 0x10, //4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
    0xF0, 0x10, 0x20, 0x40, 0x40, //7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
    0xF0, 0x90, 0xF0, 0x90, 0x90, //A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
    0xF0, 0x80, 0x80, 0x80, 0xF0, //C
    0xE0, 0x90, 0x90, 0x90, 0xE0, //D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
    0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};

uint8_t step;
uint8_t pixel;
uint16_t index;

uint8_t _y;
uint8_t _x;

void initialize() {
	opcode = I = sp = delay_timer = sound_timer = 0;
	pc = 0x200;
	
	canvas_data[0] = 64;
	canvas_data[1] = 32;
	memset(canvas_data + 2, 0, 2048);
	memset(keys, 0, 16);
	memset(stack, 0, 16);
	memset(V, 0, 16);
	memset(memory, 0, 4096);
	
	memcpy(memory, fontset, 80);
	
	srand(rtc_Time());
}

void loadProgram(char *fileName) {
	int i;
	int romSize;
	
	file = ti_Open(fileName, "r");
	ti_Read(&game_data, ti_GetSize(file), 1, file);
	
	romSize = ti_GetSize(file);
	dbg_sprintf(dbgout, "%d ", romSize);
	
	initialize();
	
	if((4096-512) > romSize) {
		for(i = 0; i < romSize; ++i) {
			memory[i + 512] = (uint8_t)game_data[i+6];
		}
	}
}

void setKeys() {
	keys[0] = kb_Data[4] & kb_DecPnt;
	keys[1] = kb_Data[3] & kb_7;
	keys[2] = kb_Data[4] & kb_8;
	keys[3] = kb_Data[5] & kb_9;
	
	keys[4] = kb_Data[3] & kb_4;
	keys[5] = kb_Data[4] & kb_5;
	keys[6] = kb_Data[5] & kb_6;
	keys[7] = kb_Data[3] & kb_1;
	
	keys[8] = kb_Data[4] & kb_2;
	keys[9] = kb_Data[5] & kb_3;
	keys[0xA] = kb_Data[3] & kb_0;
	keys[0xB] = kb_Data[5] & kb_Chs;
	
	keys[0xC] = kb_Data[6] & kb_Mul;
	keys[0xD] = kb_Data[6] & kb_Sub;
	keys[0xE] = kb_Data[6] & kb_Add;
	keys[0xF] = kb_Data[6] & kb_Enter;
}

void emulateCycle(uint8_t steps) {
	
	kb_Scan();
	setKeys();
	
	for(step = 0; step < steps; ++step) {
		int i;
		opcode = (memory[pc] << 8) | memory[pc+1];
		
		pc += 2;
		
		switch(opcode & 0xf000) {
			case 0x0000: {
				switch(opcode & 0x000f) {
					case 0x0000:
						memset(canvas_data + 2, 0, 2048);
						drawFlag = true;
						break;
					case 0x000e:
						pc = stack[(--sp)&0xf];
						break;
					default:
						pc = (pc & 0x0fff);
						break;
				}
				break;
			}
			case 0x1000: {
				pc = (opcode & 0x0fff);
				break;
			}
			case 0x2000: {
				stack[sp++] = pc;
				pc = (opcode & 0x0fff);
				break;
			}
			case 0x3000: {
				if(V[(opcode & 0x0f00) >> 8] == (opcode & 0x00ff))
					pc += 2;
				break;
			}
			case 0x4000: {
				if(V[(opcode & 0x0f00) >> 8] != (opcode & 0x00ff))
					pc += 2;
				break;
			}
			case 0x5000: {
				if(V[(opcode & 0x0f00) >> 8] == V[(opcode & 0x00f0) >> 4] && (opcode & 0x000f) == 0)
					pc += 2;
				break;
			}
			case 0x6000: {
				V[(opcode & 0x0f00) >> 8] = (opcode & 0x00ff);
				break;
			}
			case 0x7000: {
				V[(opcode & 0x0f00) >> 8] += (opcode & 0x00ff);
				break;
			}
			case 0x8000: {
				switch(opcode & 0x000f) {
					case 0x0000: {
						V[(opcode & 0x0f00) >> 8]  = V[(opcode & 0x00f0) >> 4];
						break;
					}
					case 0x0001: {
						V[(opcode & 0x0f00) >> 8] |= V[(opcode & 0x00f0) >> 4];
						break;
					}
					case 0x0002: {
						V[(opcode & 0x0f00) >> 8] &= V[(opcode & 0x00f0) >> 4];
						break;
					}
					case 0x0003: {
						V[(opcode & 0x0f00) >> 8] ^= V[(opcode & 0x00f0) >> 4];
						break;
					}
					case 0x0004: {
						const uint8_t x = (opcode & 0x0f00) >> 8;
						const uint8_t y = (opcode & 0x00f0) >> 4;
						V[0xf] = (V[x] + V[y] > 0xff);
						V[x] += V[y];
						break;
					}
					case 0x0005: {
						const uint8_t x = (opcode & 0x0f00) >> 8;
						const uint8_t y = (opcode & 0x00f0) >> 4;
						V[0xf] = V[x] > V[y];
						V[x] -= V[y];
						break;
					}
					case 0x0006: {
						const uint8_t x = (opcode & 0x0f00) >> 8;
						const uint8_t y = (opcode & 0x00f0) >> 4;
						V[0xf] = V[x] & 1;
						V[x] >>= 1;
						break;
					}
					case 0x0007: {
						const uint8_t x = (opcode & 0x0f00) >> 8;
						const uint8_t y = (opcode & 0x00f0) >> 4;
						V[0xf] = V[y] > V[x];
						V[x] = V[y] - V[x];
						break;
					}
					case 0x000E: {
						const uint8_t x = (opcode & 0x0f00) >> 8;
						const uint8_t y = (opcode & 0x00f0) >> 4;
						V[0xf] = V[x] >> 7;
						V[x] <<= 1;
						break;
					}
					break;
				}
			}
			case 0x9000: {
				if(V[(opcode & 0x0f00) >> 8] != V[(opcode & 0x00f0) >> 4] && (opcode & 0x000f) == 0)
					pc += 2;
				break;
			}
			case 0xa000: {
				I = (opcode & 0x0fff);
				break;
			}
			case 0xb000: {
				pc = V[0] + (opcode & 0x0fff);
				break;
			}
			case 0xc000: {
				V[(opcode & 0x0f00) >> 8] = (rand() % 256) & (opcode & 0x00FF);
				break;
			}
			case 0xd000: {
				const uint8_t x = V[(opcode & 0x0f00) >> 8];
				const uint8_t y = V[(opcode & 0x00f0) >> 4];
				const uint8_t height = (opcode & 0x000f);
				
				V[0xf] = 0;
				
				for(_y = 0; _y < height; ++_y) {
					pixel = memory[I + _y];
					for(_x = 0; _x < 8; ++_x) {
						if((pixel & (0x80 >> _x)) != 0) {
							index = (((x + _x) + ((y + _y) << 6)) % 2048) + 2;
							V[0xf] |= canvas_data[index] & 1;
							canvas_data[index] = ~canvas_data[index];
						}
					}
				}
				
				drawFlag = true;
				
				break;
			}
			case 0xe000: {
				switch(opcode & 0x00ff) {
					case 0x009e: {
						if(keys[V[(opcode & 0x0f00) >> 8]])
							pc += 2;
						break;
					}
					case 0x00a1: {
						if(!keys[V[(opcode & 0x0f00) >> 8]])
							pc += 2;
						break;
					}
				}
				break;
			}
			case 0xf000: {
				switch(opcode & 0x00ff) {
					case 0x0007: {
						V[(opcode & 0x0f00) >> 8] = delay_timer;
						break;
					}
					case 0x000A: {
						bool key_pressed = false;
						pc -= 2;
						
						for(i = 0; i < 16; ++i) {
							if(keys[i]) {
								V[(opcode & 0x0f00) >> 8] = i;
								pc += 2;
								key_pressed = true;
								break;
							}
						}
						
						if(!key_pressed)
							return;
					}
					case 0x0015: {
						delay_timer = V[(opcode & 0x0f00) >> 8];
						break;
					}
					case 0x0018: {
						sound_timer = V[(opcode & 0x0f00) >> 8];
						break;
					}
					case 0x001E: {
						const uint8_t x = (opcode & 0x0f00) >> 8;
						//V[0xf] = (I > 0xfff - V[x]);
						I += V[x];
						break;
					}
					case 0x0029: {
						I = V[(opcode & 0x0f00) >> 8] * 5;
						break;
					}
					case 0x0033: {
						const uint8_t x = (opcode & 0x0f00) >> 8;
						memory[ I ] =  V[x] / 100;
						memory[I+1] = (V[x] / 10) % 10;
						memory[I+2] = V[x] % 10;
						break;
					}
					case 0x0055: {
						const uint8_t x = (opcode & 0x0f00) >> 8;
						for(i = 0; i <= x; ++i) {
							memory[I + i] = V[i];
						}
						//I += x + 1;
						break;
					}
					case 0x0065: {
						const uint8_t x = (opcode & 0x0f00) >> 8;
						for(i = 0; i <= x; ++i) {
							V[i] = memory[I + i];
						}
						//I += x + 1;
						break;
					}
				}
				break;
			}
			default:
				break;
		}
		if(sound_timer > 0) {
			--sound_timer;
		}
		if(delay_timer > 0) {
			--delay_timer;
		}
	}
}
















