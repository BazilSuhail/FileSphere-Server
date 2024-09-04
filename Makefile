CC = gcc

TARGET = main.exe

SRC = server.c
 
all: $(TARGET) 
$(TARGET): $(SRC)
	$(CC) -o $(TARGET) $(SRC)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
