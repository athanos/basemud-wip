# $Id $


COLOR = \x1B[36m
CC = gcc
RM = rm
EXE = basemud
PROJECT := $(shell echo $(EXE) | tr A-Z a-z)
C_FLAGS = -Wall -pedantic -Wno-char-subscripts -Wno-unused-variable \
	  -Wno-pointer-sign -Wno-format-truncation -Wno-format-overflow \
	  -Wno-maybe-uninitialized
#C_FLAGS = -Wall
L_FLAGS = -lz -lpthread -lcrypt

# Source Files
SRC_FILES := $(wildcard *.c)

# Object Files
OBJ_DIR = obj
OBJ_FILES := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

BACKUP_DIRS := src
BACKUP_FILE := backup/$(PROJECT)-$(shell date +%Y-%m-%d).tar.gz

all: $(OBJ_FILES)
	@$(CC) -o $(EXE) $(OBJ_FILES) $(L_FLAGS)
	@echo -e "$(COLOR)$(EXE) successfully compiled."

$(OBJ_DIR)/%.o: %.c
	@echo "Compiling $<"
	@$(CC) -c $(C_FLAGS) -o $@ $<

backup:	clean
	@echo "Backing up: $(BACKUP_DIRS)"
	@(cd ..; tar -zcf $(BACKUP_FILE) $(BACKUP_DIRS))
	@echo -e "$(COLOR)New backup created: $(BACKUP_FILE)$(NOCOLOR)"

clean:
	@rm -f $(EXE)
	@rm -f $(OBJ_FILES) *~ *.bak *.orig *.rej
	@echo "Basemud source files cleaned"
