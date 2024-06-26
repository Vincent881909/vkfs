# vkFS
vkFS is a Hybrid Key-Value Store File System leveraging RocksDB. This artificat is part of my Bachelor Thesis work entitled: "Optimizing Metadata Handling with vkFS: A Hybrid Key-Value Store File System leveraging RocksDB".


## vkFS Design
![vkFS-Architecture drawio](https://github.com/Vincent881909/vkfs/assets/64153237/d65678b6-d101-47d8-8f97-d9e303981148)

vkFS stored metadata and files smaller than 4KiB in a key-value store. File larger then 4KiB are stored in the underlying file system. 

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

### 6. Configure Fuse (Optional)
When running Filebench we need to allow other users than the user that mounted the file system to access it. Since Filebench requires sudo priveliges we need to enable the -allowother flag.
Open the following file, and uncomment the flag by removing the #
````bash
sudo nano /etc/fuse.conf
````

### 7. Prepare Directories
Create the following directories
````bash
mkdir bin mntdir metadir datadir
````
### 8. Compile and run the Project
Alternatively, you can run make run_debug to enable debug print statements
```bash
make
make run
````

## Setup Benchmarking Tools
### 1. Install Filebench Benchmarking
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

### 2. Disable Virtualization
Run the following command to disable Address Space Layout Randomization (ASLR) before using Filebench:
````bash
echo 0 | sudo tee /proc/sys/kernel/randomize_va_space
````

## Running vkFS
Once you have installed vkFS as described above you can start running the file system by using the following make command:
````bash
make run
````
Alternatively, you can also run the program in debug mode. This will print every FUSE operation called along with its arguments. You can run vkFS in debug mode with the following command:
````bash
make run_debug
````

Open a new terminal window and navigate to the "mntdir" directory. Now you can use interact with the file system just like any other file system.

## Run Benchmarks
You can run all benchmarks by first starting the program, and then navigating in a new terminal window to the "vkfs" directory and run the following command:
````bash
make micro_benchmarks
````
This will run all micro benchmarks in sequence. Additionally, if you wantto run the macro benchmarks, you can run the following command:
````bash
make macro_benchmarks
````

To run all benchmarks in sequence run:
````bash
make micro_benchmarks && make macro_benchmarks
````

## License
vkFS comes with a MIT license
 
