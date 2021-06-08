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

### (Info) looking around in the cobra image
```
docker run --rm -it --entrypoint bash cobra
root@f323c223eaa0:/var/evalrun# ls -al
total 8
drwxr-xr-x 2 root root 4096 Jun  7 14:15 .
drwxr-xr-x 1 root root 4096 Jun  7 14:15 ..
root@f323c223eaa0:/var/evalrun# cd /opt/cobra/build
root@f323c223eaa0:/opt/cobra/build# ls -al
total 36004
drwxr-xr-x 1 root root    4096 Jun  7 14:15 .
drwxr-xr-x 1 root root    4096 Jun  7 13:49 ..
-rw-r--r-- 1 root root   17077 Jun  7 13:56 CMakeCache.txt
drwxr-xr-x 1 root root    4096 Jun  7 14:15 CMakeFiles
-rw-r--r-- 1 root root  372259 Jun  7 13:56 Makefile
drwxr-xr-x 4 root root    4096 Jun  7 13:49 _3rdParty
-rw-r--r-- 1 root root    1337 Jun  7 13:56 cmake_install.cmake
-rwxr-xr-x 1 root root 5189224 Jun  7 13:59 cobra-copy
-rwxr-xr-x 1 root root 5296720 Jun  7 14:15 cobra-evaluate
-rwxr-xr-x 1 root root 5193640 Jun  7 14:01 cobra-insert
-rwxr-xr-x 1 root root 5189264 Jun  7 14:09 cobra-query-delta-materialized
-rwxr-xr-x 1 root root 5194496 Jun  7 14:07 cobra-query-version
-rwxr-xr-x 1 root root 5189264 Jun  7 14:12 cobra-query-version-materialized
-rwxr-xr-x 1 root root 5193536 Jun  7 14:04 cobra-reverse
root@f323c223eaa0:/opt# cd /opt/patchstore
bash: cd: /opt/patchstore: No such file or directory
root@f323c223eaa0:/opt# 
```

- There are `cobra-*` commands, similar to the `ostrich-*` commands in the ostrich image
- There is no `cobra_test` command, similar to the `ostrich_test`command mentioned [here](https://github.com/rdfostrich/ostrich#test)

## Put the data and queries in place

TODO
(Try to proceed as documented in https://github.com/rdfostrich/ostrich#docker, replacing 'ostrich' with 'cobra).

## Run the experiments

TODO
(Try to proceed as documented in https://github.com/rdfostrich/ostrich#docker, replacing 'ostrich' with 'cobra).

