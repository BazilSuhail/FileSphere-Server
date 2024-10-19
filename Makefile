CC = gcc

TARGET = main.exe

SRC = server.c
QUERIES = helper_queries.c 
AUTH = helper_authentication.c
PARSE = helper_parsingConfig.c
 
all: $(TARGET) 
$(TARGET): $(SRC)
	$(CC) -o $(TARGET) $(SRC) $(QUERIES) $(AUTH) $(PARSE)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
