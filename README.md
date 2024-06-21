# vkFS
vkFS is a Hybrid Key-Value Store File System leveraging RocksDB. This artificat is part of my Bachelor Thesis work entitled: "Optimizing Metadata Handling with vkFS: A Hybrid Key-Value Store File System leveraging RocksDB".

## Installation Guide

### 1. Clone the Repository
````bash
git clone https://github.com/Vincent881909/vkfs.git
cd vkfs
````

### 2. Install Dependencies

Use the following command to install the necessary packages:

```bash
sudo apt-get update
sudo apt-get install -y \
  g++ \
  build-essential \
  fuse3 \
  libfuse3-3 \
  libfuse3-dev \
  zlib1g-dev \
  libbz2-dev \
  liblz4-dev \
  libsnappy-dev \
  libzstd-dev \
  bison \
  flex \
  libtool \
  automake
  ```

 ### 3. Install Make
 To install make from source, follow these steps:
 ````bash
wget https://ftp.gnu.org/gnu/make/make-4.3.tar.gz
tar -xzvf make-4.3.tar.gz
cd make-4.3
./configure
make
sudo make install
````

### 4. Initialize Submodule
````bash
git submodule update --init --recursive
````
### 5. Install RocksDB
To install RocksDB, run the following commands
````bash
cd rocksdb/
make static_lib
cd ../
````

### 6. Install Filebench Benchmarking (Optional)
If you want to test the file system using Filebench run the following commands:
````bash
wget https://github.com/filebench/filebench/archive/refs/tags/1.4.9.1.tar.gz
tar -xzvf 1.4.9.1.tar.gz
cd filebench-1.4.9.1
libtoolize
aclocal
autoheader
automake --add-missing
autoconf
./configure
make
sudo make install
cd ..
rm -rf 1.4.9.1.tar.gz
````

### 7. Disable Virtualization (Optional)
Run the following command to disable Address Space Layout Randomization (ASLR) before using Filebench:
````bash
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
````

### 8. Configure Fuse (Optional)
When running Filebench we need to allow other users than the user that mounted the file system to access it. Since Filebench requires sudo priveliges we need to enable the -allowother flag.
Open the following file, and uncomment the flag by removing the #
````bash
sudo nano /etc/fuse.conf
````

### 9. Prepare Directories
Create the following directories
````bash
mkdir bin mntdir metadir datadir
````
### 10. Compile and run the Project
Alternatively, you can run make run_debug to enable debug print statements
```bash
make
make run
````

 
