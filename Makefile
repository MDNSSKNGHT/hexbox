CFLAGS += -Wall -Werror -Wextra
CFLAGS += -g3
OBJ += hexbox.o

all: hexbox

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: hexbox clean

hexbox: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -rf hexbox $(OBJ)
