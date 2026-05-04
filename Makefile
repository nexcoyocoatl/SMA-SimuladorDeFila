# Linux e Windows

SRC_DIR = src/
INC_DIR = include/
SRC = $(wildcard $(SRC_DIR)*.c)
OBJ = $(SRC:.c=.o)

CFLAGS = -std=c99 -O0 -I$(INC_DIR)
DBGFLAGS = -Wall -Werror -pedantic -g
LDFLAGS =
CC = gcc
EXEC =

ifdef OS
   BIN = simulate_queue.exe
   fix_path = $(subst /,\,$1)
   RM = del /q
else
   ifeq ($(shell uname), Linux)
      BIN = simulate_queue
      fix_path = $1
      RM = rm -f
	   EXEC = ./
   endif
endif

# Default
all: $(BIN)

# Linking
$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(DBGFLAGS) $(OBJ) -o $(BIN) $(LDFLAGS)

# Compilation
$(SRC_DIR)%.o: $(SRC_DIR)%.c
	$(CC) $(CFLAGS) $(DBGFLAGS) -c $< -o $@


run: all
	$(EXEC)$(BIN)

clean:
	-@ $(RM) $(call fix_path,$(OBJ)) $(BIN)
