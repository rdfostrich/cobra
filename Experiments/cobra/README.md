# Reproducing COBRA experiments

This documentation is as good as identical to the documentation for the OSTRICH experiment.
The few differences are marked with !!DIFF!! in the OSTRICH documentation.

Execute all commands shown below in this directory of a working copy cloned from this repository
(unless explicitly stated otherwise).

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

This step is needed to run locally. When running on server donizetti.labnet, the data is already there.

**TODO: Document how to create input, starting from [the BEAR documentation](https://aic.ai.wu.ac.at/qadlod/bear.html).**

```sh
# create directories
sudo mkdir /mnt/datastore
sudo chmod 777 /mnt/datastore/
mkdir -p /mnt/datastore/data/dslab/experimental/patch
# copy data; select:
# - beara, only versions 1..10
for i in {1..10}; do rsync -rtvz donizetti.labnet:/mnt/datastore/data/dslab/experimental/patch/data/$i /mnt/datastore/data/dslab/experimental/patch/data ; done
# - bearb-day, full
rsync -rtvz donizetti.labnet:/mnt/datastore/data/dslab/experimental/patch/rawdata-bearb/patches-day /mnt/datastore/data/dslab/experimental/patch/rawdata-bearb
# - bearb-hour, full
rsync -rtvz donizetti.labnet:/mnt/datastore/data/dslab/experimental/patch/rawdata-bearb/patches-hour /mnt/datastore/data/dslab/experimental/patch/rawdata-bearb
# copy queries
rsync -rtvz donizetti.labnet:/mnt/datastore/data/dslab/experimental/patch/BEAR/queries_new /mnt/datastore/data/dslab/experimental/patch/BEAR
rsync -rtvz donizetti.labnet:/mnt/datastore/data/dslab/experimental/patch/BEAR/queries_bearb /mnt/datastore/data/dslab/experimental/patch/BEAR

popd
```

## Run the experiments

**TODO: what follows is for *ostrich* . Finally, copy to *ostrich below* and adapt this doc for *cobra*.**

Note - on the server you may want to do this in a **screen** session.
```sh
./run-docker.sh beara 2>&1 | tee beara.log
./run-docker.sh bearb-day 2>&1 | tee bearb-day.log
./run-docker.sh bearb-hour 2>&1 | tee bearb-hour.log
```
