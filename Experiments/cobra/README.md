# Reproducing COBRA experiments

## Prerequisites

### Docker

Have [docker](https://docs.docker.com/get-docker/) installed and [get access as non-root user](https://docs.docker.com/engine/install/linux-postinstall/#manage-docker-as-a-non-root-user).

### Submodules

If you didn't clone this repo with the `--recurse-submodules` option in your `git clone` command, get the required submodules now,
by executing this in the root of your working copy:

```
git submodule update --init
```

## Create docker image

Execute in the directory containing this README, in a working copy cloned from this repository.

```sh
docker build -t cobra .
```

## Get input data and queries

The input data and queries can be get as explained for HDT.

In addition, some query files need to be split in lines:
```
cd /mnt/datastore/data/dslab/experimental/patch/BEAR/queries_new/
/path/to/working-copy/utils/split-in-lines.sh
```
(where `/path/to/working-copy/` is the path to the root of a working copy of this repo)

## Put the data and queries in place

This step is needed to run locally, assuming the data and queries at `donizetti.labnet:/mnt/datastore/data/dslab/experimental/patch/` are golden.

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

Execute in the directory containing this README, in a working copy cloned from this repository.

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
./run-docker.sh ingest bearb-day  pre_fix_up 2>&1 | tee ingest-bearb-day-pre_fix_up.log
./run-docker.sh ingest bearb-day  fix_up     2>&1 | tee ingest-bearb-day-fix_up.log
./run-docker.sh ingest bearb-hour pre_fix_up 2>&1 | tee ingest-bearb-hour-pre_fix_up.log
./run-docker.sh ingest bearb-hour fix_up     2>&1 | tee ingest-bearb-hour-fix_up.log
./run-docker.sh ingest beara      pre_fix_up 2>&1 | tee ingest-beara-pre_fix_up.log
#./run-docker.sh ingest beara      fix_up     2>&1 | tee ingest-beara-fix_up.log
```

Note: to join output of queries on split query files, use the `/utils/join-xyzt.sh` script of this project.

## Copy the data to this repo

We can copy the data on the server (donizetti) to this repo, in a subdirectory `Results/output` of the current folder
using the bash script `./get-output.sh`.
