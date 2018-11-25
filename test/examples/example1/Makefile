CFLAGS 	= -Wall -O3 -fPIC -g

# Directories automatically crated
TMP_DIR	= .dptmp

LDLIB	= -L/home/wb/WBTuner/dplib -L./
LDFLAGS	= -lDpLib -lm

INCPATH	= -I/home/wb/WBTuner/dplib \
	-I./

SRC 	= example1.c

.PHONY: all

all:
	@echo Final linking
	gcc $(SRC) $(LDLIB) -o run $(LDFLAGS) $(INCPATH)
clean:
	rm -f $(TMP_DIR)/* run
