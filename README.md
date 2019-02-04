# AFLSmart: Smart Greybox Fuzzing

AFLSmart is an extension of [AFL](http://lcamtuf.coredump.cx/afl/) from Michał Zalewski. It makes AFL input-structure aware by taking in high-level structural representation of seed files. It combines AFL with Peach input model/grammar definitions. We thank Peach Tech for making [Peach Community version](http://www.peach.tech/resources/peachcommunity/) open source. 

The work was done by [Van-Thuan Pham](https://thuanpv.github.io/), [Marcel Böhme](https://mboehme.github.io/), Andrew E. Santosa, Alexandru Răzvan Căciulescu, and [Abhik Roychoudhury](https://www.comp.nus.edu.sg/~abhik/).

For more details, please checkout our preprint: [Smart Greybox Fuzzing](https://arxiv.org/pdf/1811.09447.pdf).

# Installation

## Prerequisites

Install automake and some required packages
```bash
sudo apt-get install automake libtool libc6-dev-i386 python-pip g++-multilib
```

Compile and install mono package to support C# on Linux
```bash
sudo apt-get install mono-complete
```
Install gcc-4.4 and g++-4.4
```bash
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt install gcc-4.4
sudo apt install g++-4.4
```

## AFLSmart

Download AFLSmart and compile it.
```
git clone https://github.com/aflsmart/aflsmart
cd aflsmart
make clean all
cd ..

export AFLSMART=$(pwd)/aflsmart
export WORKDIR=$(pwd)
```

## Modified version of Peach

```bash
cd $AFLSMART
wget https://sourceforge.net/projects/peachfuzz/files/Peach/3.0/peach-3.0.202-source.zip
unzip peach-3.0.202-source.zip
patch -p1 < peach-3.0.202.patch
cd peach-3.0.202-source
CC=gcc-4.4 CXX=g++-4.4 ./waf configure
CC=gcc-4.4 CXX=g++-4.4 ./waf install
```

## Setup PATH environment variables

```bash
export PATH=$PATH:$AFLSMART:$AFLSMART/peach-3.0.202-source/output/linux_x86_64_debug/bin
export AFL_PATH=$AFLSMART
export LD_LIBRARY_PATH=/usr/local/lib
```

# Usage

AFLSmart adds four more options to AFL

-w: input model type. AFLSmart currently only supports Peach.

-g: input model file. Path to the input model file (a.k.a Peach pit) is required. We have provided 10 sample Peach pits in the input_models folder. To write a new Peach pit for a new file format, please follow [this tutorial](http://community.peachfuzzer.com/v3/PeachQuickStart.html) and revisit [Section 4 - File Format Specification](https://arxiv.org/pdf/1811.09447.pdf) of the AFLSmart paper.

-h: stacking mutations mode which mixes normal and higher-order mutation operators together. 

-H: limit the number of higher-order mutations for each input. This is an optional option; there is no limit if the option is not set.

Example command: 
```bash
afl-fuzz -h -i in -o out -w peach -g <input model file> -x <dictionary file> <executable binary and its arguments> @@
```

During the fuzzing process, AFLSmart will interact with Peach to get the validity and chunks' boundary information. Please check the out/chunks folder and make sure that it is not empty. If it is empty, Peach executable may not be found and you need to compile Peach and/or check the PATH environment variable.

# Examples

To fuzz WavPack and reproduce CVE-2018-10536. See [Section 2 - Motivating Example](https://arxiv.org/pdf/1811.09447.pdf) in the AFLSmart paper.

## Patch afl-fuzz.c to make it work with WavPack as WavPack only takes inputs having correct file extension (e.g., .wav)
```bash
cd $AFLSMART
sed -i 's/cur_input/cur_input.wav/g' afl-fuzz.c
make clean all
```

## Compile the vulnerable version of WavPack
```bash
cd $WORKDIR
git clone https://github.com/dbry/WavPack.git
cd WavPack
git checkout 0a72951
./autogen.sh
CC=afl-gcc ./configure --disable-shared
make clean all
```
## Fuzz it in 24 hrs
```bash
cd $WORKDIR/WavPack
timeout 24h $AFLSMART/afl-fuzz -m none -h -d -i $AFLSMART/testcases/aflsmart/wav -o out -w peach -g $AFLSMART/input_models/wav.xml -x $AFLSMART/dictionaries/wav.dict cli/wavpack -y @@ -o out
```

## Trophy case
We would love to hear from you if you have found interesting vulnerabilities with AFLSmart

* FFmpeg: 10 bugs reported, 9 CVEs assigned (CVE-2018-12458, CVE-2018-12459, CVE-2018-12460, CVE-2018-13300, CVE-2018-13301, CVE-2018-13302, CVE-2018-13303, CVE-2018-13304, CVE-2018-13305)
* PDFium: Issue-912846
* LibPNG: CVE-2018-13785
* Binutils: 4 bugs reported, 2 CVEs assigned (CVE-2018-10372, CVE-2018-10373)
* OpenJPEG: 3 bugs reported
* Jasper: 11 bugs reported, 5 CVEs assigned (CVE-2018-19539, CVE-2018-19540, CVE-2018-19541, CVE-2018-19542, CVE-2018-19543)
* LibAV: 6 bugs reported
* WavPack: CVE-10536, CVE-10537, CVE-10538, CVE-10539, CVE-10540

## Contributions

All contributions are welcome. We would love to get your pull requests for bug fixes, improvements and new input models. We have provided 10 Peach pits for popular file fomrats (e.g., PDF, PNG, AVI ...) and will try to gradually upload more to the repository but there are hundreds of file formats out there and it would be great if AFLSmart's users could contribute their models.
