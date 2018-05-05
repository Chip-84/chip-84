#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>
#include <debug.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern uint16_t opcode;
extern uint8_t memory[4096];
extern uint8_t V[16];
extern uint16_t I;
extern uint16_t pc;
extern int16_t delay_timer;
extern int16_t sound_timer;
extern uint16_t stack[16];
extern uint8_t sp;
extern uint8_t keys[16];
extern bool drawFlag;

extern ti_var_t file;

extern uint8_t game_data[3584];

void loadFontset(void);
void initialize(void);
void loadProgram(char *fileName);
void emulateCycle(uint8_t steps);
void setKeys(void);