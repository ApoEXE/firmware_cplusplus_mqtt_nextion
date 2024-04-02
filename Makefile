#all: main.cpp nextion_driver.o
#	g++ -o test -std=c++11  main.cpp -I./Nextion_driver/ -Isrc/ -Wall -lpthread -lpaho-mqtt3a


# Define the name of your executable
TARGET = test

# Define the C compiler (adjust if needed)
CC = g++


# Define compiler flags (feel free to add optimization flags)
CFLAGS = -Wall -L/usr/local/lib/libpaho-mqtt3a.so -lpaho-mqtt3a -pthread 

# Define the source file in the root directory
MAIN_SRC = main.cpp

# Define the source file in the subdirectory
SUBDIR_SRC = Nextion_driver/nextion_driver.cpp

# Define object files (automatically generated)
OBJECTS = $(MAIN_SRC:.cpp=.o) $(SUBDIR_SRC:.cpp=.o)

# Define all dependency files
DEPS = $(OBJECTS)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC)  -o $(TARGET) $(OBJECTS) $(CFLAGS)

# Pattern rule for compiling C source files
%.o: %.cpp
	$(CC)  -c $< -o $@ $(CFLAGS)

clean:
	rm -f $(OBJECTS) $(TARGET)