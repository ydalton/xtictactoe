CPPFLAGS 	:= $(shell pkg-config --cflags x11)
CFLAGS 		:= -O2 -Wall -Wextra -Wpedantic
LDFLAGS 	:= $(shell pkg-config --libs x11) -lm
OBJ 		:= main.o
BIN 		:= xtictactoe

$(BIN): $(OBJ)
	$(CC) -o $(BIN) $(OBJ) $(LDFLAGS)

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: $(BIN) clean
