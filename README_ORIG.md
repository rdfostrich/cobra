# COBRA
_Change-based Offset-enabled Bidirectional RDF Archive._

[![Build Status](https://travis-ci.org/rdfostrich/cobra.svg?branch=master)](https://travis-ci.org/rdfostrich/cobra)
[![Docker Automated Build](https://img.shields.io/docker/automated/rdfostrich/cobra.svg)](https://hub.docker.com/r/rdfostrich/cobra/)

**COBRA** is a bidirectional extension of [**OSTRICH**](https://github.com/rdfostrich/ostrich/),
an _RDF triple store_ that allows _multiple versions_ of a dataset to be stored and queried at the same time.

**Warning: this is experimental software**

The store is a hybrid between _snapshot_, _delta_ and _timestamp-based_ storage,
which provides a good trade-off between storage size and query time.
It provides several built-in algorithms to enable efficient iterator-based queries _at_ a certain version, _between_ any two versions, and _for_ versions. These queries support limits and offsets for any triple pattern.

Insertion is done by first inserting a dataset snapshot, which is encoded in [HDT](rdfhdt.org).
After that, deltas can be inserted, which contain additions and deletions based on the last delta or snapshot.

More details on COBRA can be found in our [article](https://rdfostrich.github.io/article-odbase2020-cobra/).
More details on OSTRICH can be found in our [journal](https://rdfostrich.github.io/article-jws2018-ostrich/) or [demo](https://rdfostrich.github.io/article-demo/) articles.

## Building

COBRA requires ZLib, Kyoto Cabinet and CMake (compilation only) to be installed.

Compile:
```bash
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## Running

The COBRA dataset will always be loaded from the current directory.

_For more information, please refer to the [OSTRICH documentation](https://github.com/rdfostrich/ostrich)._

## Compiler variables
`PATCH_INSERT_BUFFER_SIZE`: The size of the triple parser buffer during patch insertion. (default `100`)

`FLUSH_POSITIONS_COUNT`: The amount of triples after which the patch positions should be flushed to disk, to avoid memory issues. (default `500000`)

`FLUSH_TRIPLES_COUNT`: The amount of triples after which the store should be flushed to disk, to avoid memory issues. (default `500000`)

`KC_MEMORY_MAP_SIZE`: The KC memory map size per tree. (default `1LL << 27` = 128MB)

`KC_PAGE_CACHE_SIZE`: The KC page cache size per tree. (default `1LL << 25` = 32MB)

`MIN_ADDITION_COUNT`: The minimum addition triple count so that it will be stored in the db. Changing this value only has effect during insertion time. Lookups are compatible with any value. (default `200`)

## License
This software is written by Thibault Mahieu and [Ruben Taelman](http://rubensworks.net/) and colleagues.

This code is copyrighted by [Ghent University – imec](http://idlab.ugent.be/)
and released under the [MIT license](http://opensource.org/licenses/MIT).

