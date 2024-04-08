CC = g++
CFLAGS = -Wall `pkg-config fuse --cflags`
LDFLAGS = `pkg-config fuse --libs`

SRC_DIR = src
TARGET = $(SRC_DIR)/hybridfs

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

MOUNTPOINT=mntdir
METADATA_DIR=metadir
DATA_DIR=datadir


all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(SRC_DIR)/*.o $(TARGET)

.PHONY: run
run:
	./src/hybridfs $(MOUNTPOINT) $(METADATA_DIR) $(DATA_DIR)


