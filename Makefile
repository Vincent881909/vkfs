CXX=g++
CXXFLAGS=-std=c++17 -I/usr/local/include
LDFLAGS=-L/usr/local/lib -lrocksdb -lpthread -lsnappy -lgflags -lz -lbz2 -llz4 -lzstd
SRC=$(wildcard *.cpp)
OBJ=$(SRC:.cpp=.o)
EXEC=myapp

# Default target
all: $(EXEC)

# Compile and link
$(EXEC): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean run

# Remove object files and the executable
clean:
	rm -f $(OBJ) $(EXEC)

# Compile and run the program
run: $(EXEC)
	./$(EXEC)

# New target to compile and then run
compile_and_run: all run
