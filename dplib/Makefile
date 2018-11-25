CC = gcc
CFLAGS = -Wall -O3 -c -fPIC -g
SHARED_FLAGS = -shared
LD_FLAGS = -lrt

SRC = DpLib.c \
			DpUserApi.c \
			SamplingIO.c \
			SamplingStg.c \
			AggregateStg.c

OBJ = $(patsubst %.c, %.o, $(SRC))

all: $(OBJ) Sched.o
	@echo Building library
	$(CC) $(SHARED_FLAGS) -o libDpLib.so $(OBJ) Sched.o $(LD_FLAGS)
#ar rcs libdplib.a $(OBJ)

%.o: %.c
	@echo Compiling $*.c
	$(CC) $(CFLAGS) $< -o $@

Sched.o: Sched.c
	$(CC) -Wall -O0 -c -fPIC -g Sched.c -o Sched.o

.PHONY: all clean

clean:
	rm -f *.o libDpLib.so
