CC = gcc
OUT_DIR = build
OUT = comp
SRC = lang/muon.c
FLAGS = -Wall

MKDIR_P = mkdir -p

all: $(OUT_DIR)
	$(CC) -o $(OUT_DIR)/$(OUT) $(FLAGS) $(SRC)

debug: $(OUT_DIR)
	$(CC) -g -o $(OUT_DIR)/$(OUT) $(FLAGS) $(SRC)


clean: 
	rm -rf $(OUT_DIR)/*

$(OUT):
	$(MKDIR_P) $(OUT_DIR)
