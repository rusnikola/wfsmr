# Crystalline Reclamation

## Source Code

This code further extends the existent Interval-Based-Reclamation benchmark.
Crystalline's code is directly embedded into the benchmark. Hyaline's code in
a separate 'hyaline' directory.

## Building

To build this benchmark suite, we use clang 11.0. You can
download it from (http://releases.llvm.org/download.html). You can use
Ubuntu 16.04 pre-built binaries on Ubuntu 18.04 LTS:

https://github.com/llvm/llvm-project/releases/download/llvmorg-11.0.1/clang+llvm-11.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz

Please extract it to /opt/llvm/ (e.g., /opt/llvm/bin/clang for clang).
If extracted to a different directory, then change clang and
clang++ paths in the Makefile.

You also need to install hwloc and jemalloc. If running Ubuntu 18.04, you
can type:

sudo apt-get install build-essential libjemalloc-dev libhwloc-dev libc6-dev libc-dev make python

To compile the benchmark with Crystalline:

* Go to 'benchmark'
(This benchmark is already modified to use Crystalline.)

* Run 'make'

See the original IBR's instructions in the 'benchmark' directory.

## Unreclaimed Objects

We had to change the default method of counting uncreclaimed
objects in the benchmark, as the original approach would not work
as is with Hyaline or Crystalline due to the global retirement of objects.

However, measurements become more expensive and skew throughput
for some tests. For this reason, we introduced an extra '-c'
flag. By default, we do not count (properly) the number of
unreclaimed objects. If the -c flag is specified, we count
the number of objects (potentially skewing throughput). To count
the number of objects, you have to run a separate test with the -c flag.

## Usage Example

A sample command (not counting uncreclaimed objects):

./bin/main -i 1 -m 3 -v -r 1 -o hashmap_result.csv  -t 4 -d tracker=HyalineEL
