# ----------------------------
# Set NAME to the program name
# Set ICON to the png icon file name
# Set DESCRIPTION to display within a compatible shell
# Set COMPRESSED to "YES" to create a compressed program
# ----------------------------

NAME        ?= CHIP84
COMPRESSED  ?= NO
ICON        ?= iconc.png
DESCRIPTION ?= "CHIP8 Emu"

# ----------------------------

include $(shell cedev-config --makefile)
