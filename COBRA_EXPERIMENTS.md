# Reproducing COBRA experiments

Execute all commands shown below in this directory of a working copy cloned from this repository.

## Prerequisites

### Docker

Have [docker](https://docs.docker.com/get-docker/) installed and [get access as non-root user](https://docs.docker.com/engine/install/linux-postinstall/#manage-docker-as-a-non-root-user).

### Submodules

If you didn't clone this repo with the `--recurse-submodules` option in your `git clone` command, get the required submodules now:
```
git submodule update --init
```

## Create docker image

```sh
docker build -t cobra .
```

## Put the data and queries in place

TODO

## Run the experiments

TODO

