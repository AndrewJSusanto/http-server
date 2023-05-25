CC = clang
CFLAGS = -Wall -Werror -Wextra -pedantic
LDFLAGS = 
OBJFILES = httpserver.o
TARGET = httpserver

all = $(TARGET)

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)
	
clean:
	rm -f $(OBJFILES) $(TARGET) *~
