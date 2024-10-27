CC = gcc

TARGET = main.exe

SRC = server.c
QUERIES = helper_queries.c 
AUTH = helper_authentication.c
PARSE = helper_parsingConfig.c
SYNC = helper_synchronization.c
 
all: $(TARGET) 
$(TARGET): $(SRC)
	$(CC) -o $(TARGET) $(SRC) $(QUERIES) $(AUTH) $(PARSE) $(SYNC)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
