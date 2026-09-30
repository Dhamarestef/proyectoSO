TARGET = miproyecto
CC = gcc
all: $(TARGET)
$(TARGET): main_ncurses.c
	$(CC) main_ncurses.c -o $(TARGET) -lncurses
run: all
	./$(TARGET)
clean:
	rm -f $(TARGET)