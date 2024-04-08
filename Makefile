CC = g++
CFLAGS = -Wall `pkg-config fuse --cflags`
LDFLAGS = `pkg-config fuse --libs`

SRC_DIR = src
TARGET = $(SRC_DIR)/hybridfs

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

IMAGE=myhfs.img
MOUNTPOINT=mnt

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(SRC_DIR)/*.o $(TARGET)

.PHONY: image mount unmount
image:
	dd if=/dev/zero of=$(IMAGE) bs=1M count=512
	mkfs.ext4 $(IMAGE)

mount:
	mkdir -p mnt
	sudo mount -o loop $(IMAGE) $(MOUNTPOINT)

unmount:
	sudo umount $(MOUNTPOINT)

.PHONY: run
run:
	./src/hybridfs -i myhfs.img mnt -d


