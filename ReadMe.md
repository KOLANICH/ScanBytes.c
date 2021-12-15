ScanBytes [![Unlicensed work](https://raw.githubusercontent.com/unlicense/unlicense.org/master/static/favicon.png)](https://unlicense.org/)
=========
[![GitHub Actions](https://github.com/ScanBytes/splitindex.py/workflows/CI/badge.svg)](https://github.com/ScanBytes/splitindex.py/actions/)
[![Libraries.io Status](https://img.shields.io/librariesio/github/ScanBytes/splitindex.py.svg)](https://libraries.io/github/ScanBytes/splitindex.py)

ScanBytes is a tool and a lib for scanning files for occurrences of certain bytes fast.

It can be useful for example for creating an index of a CSV/TSV file or just a file with lines. Then a certain line/record can be quickly fetched by its index.

Features
--------

* Multithreading brings performance benefits when data fits in disk caches.
* 2 generic backends, one is JIT-ed one, another one is not-JITed. Obviously, JIT is supported only on certain platforms, currently only x86_64.
* Specialized hardcoded backend for scanning certain common cases:
  * lines in a file
  * TSV
  * CSV
* Automatic dispatching between backends.
* Built-in benchmark.

Example
-------

```bash
echo "The quick brown fox jumps over the lazy dog" > fox.txt
#     0123456789ABCDEF0123456789ABCDEF01234567
ScanBytes --alphabet " fh" s fox.txt | hd
```

```
00000000  01 00 00 00 00 00 00 00  03 00 00 00 00 00 00 00  |................|
00000010  09 00 00 00 00 00 00 00  0f 00 00 00 00 00 00 00  |................|
00000020  10 00 00 00 00 00 00 00  13 00 00 00 00 00 00 00  |................|
00000030  19 00 00 00 00 00 00 00  1e 00 00 00 00 00 00 00  |................|
00000040  20 00 00 00 00 00 00 00  22 00 00 00 00 00 00 00  | .......".......|
00000050  27 00 00 00 00 00 00 00                           |'.......|
00000058
```

As you see, there is a lot of redundancy in the output. It can be compressed by encoding it into a proper data structure, but it is currently notimplemented in C++.


Installation
------------

Packaging with CPack is implemented, you can generate an installable package for Debian and RPM-based distros.
All the dependencies are assummed to be installed the same way.


Related projects
----------------
* https://github.com/BurntSushi/rust-csv/blob/master/csv-index/src/simple.rs#L95L120
* https://github.com/waltonseymour/glossary and https://github.com/ScanBytes/glossary.py
