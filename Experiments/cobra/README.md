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

Note: the `query_news` directory has a subdirectory `split`, containing queries split in single lines. These were made using the `/utils/split-in-lines.sh` script of this project.

## Run the experiments

Note - on the server you may want to do this in a **screen** session.
```sh
# this line is optional; execute in case you don't have rwx access to the folder
sudo chown $USER:docker /mnt/datastore/data/dslab/experimental/patch/

# ingestion + querying, all bearkinds, ingestion option cobra_opt
# (in preferred order of execution)
./run-docker.sh ingest-query bearb-day  cobra_opt 2>&1 | tee ingest-query-bearb-day-cobra_opt.log
./run-docker.sh ingest-query bearb-hour cobra_opt 2>&1 | tee ingest-query-bearb-hour-cobra_opt.log
./run-docker.sh ingest-query beara      cobra_opt 2>&1 | tee ingest-query-beara-cobra_opt.log

# ingestion only, all bearkinds, ingestion options pre_fix_up and fix_up in sequence
# (in preferred order of execution)
./run-docker.sh ingest bearb-day  pre_fix_up 2>&1 | tee ingest-bearb-pre_fix_up.log
./run-docker.sh ingest bearb-day  fix_up     2>&1 | tee ingest-bearb-fix_up.log
./run-docker.sh ingest bearb-hour pre_fix_up 2>&1 | tee ingest-bearb-pre_fix_up.log
./run-docker.sh ingest bearb-hour fix_up     2>&1 | tee ingest-bearb-fix_up.log
./run-docker.sh ingest beara      pre_fix_up 2>&1 | tee ingest-beara-pre_fix_up.log
./run-docker.sh ingest beara      fix_up     2>&1 | tee ingest-beara-fix_up.log
```

Note: to join output of queries on split query files, use the `/utils/join-xyzt.sh` script of this project.

