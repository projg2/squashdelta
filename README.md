# squashdelta
Create efficient deltas (patches) between two SquashFS images

## Install dependencies
```bash
$ sudo apt install automake libboost-dev liblz4-dev xdelta3
```

## Building from source
This project uses [autotools](http://inti.sourceforge.net/tutorial/libinti/autotoolsproject.html)

```bash
$ git clone https://github.com/mgorny/squashdelta.git
$ cd squashdelta
```

```bash
$ aclocal
$ autoconf
$ touch AUTHORS NEWS README ChangeLog
$ mkdir build-aux
$ autoreconf --install
$ automake --add-missing
```

```bash
$ ./configure --prefix=/opt/bin
$ make
```

## Usage
```bash
$ ./squashdelta <source> <target> <patch-output>
```

## Whitepaper
https://dev.gentoo.org/~mgorny/articles/reducing-squashfs-delta-size-through-partial-decompression.pdf
