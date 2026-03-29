# Linux e Windows

SRC_DIR = src/
SRC = simulate_queue.c
OBJ = $(addprefix $(SRC_DIR),$(SRC:.c=.o))
CFLAGS = -std=c99
DBGFLAGS = -Wall -Werror -pedantic #-g
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

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LDFLAGS) $(DBGFLAGS) -o $(BIN)

run:
	$(EXEC)$(BIN)

clean:
	-@ $(RM) $(call fix_path,$(OBJ)) $(BIN)