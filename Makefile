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

.PHONY: fileset
fileset:
	sudo filebench -f workloads/createfiles.f

.PHONY: benchmark1
benchmark1:
	sudo filebench -f workloads/deletefiles.f

.PHONY: benchmark2
benchmark2:
	sudo filebench -f workloads/listdirs.f

.PHONY: benchmark3
benchmark3:
	sudo filebench -f workloads/statfiles.f


