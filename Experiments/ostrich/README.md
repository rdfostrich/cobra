# Reproducing OSTRICH experiments

## Prerequisites

### Docker

Have [docker](https://docs.docker.com/get-docker/) installed and [get access as non-root user](https://docs.docker.com/engine/install/linux-postinstall/#manage-docker-as-a-non-root-user).

### Additional repo

The code for Ostrich is in a separate git repository!

Clone that repo with the `--recurse-submodules` option in your `git clone` command.
**Do this in a parent directory outside your working copy of this *cobra* repo!**
```
git clone https://github.com/rdfostrich/ostrich.git --recurse-submodules
```

## Create docker image

**Do this in the root directory of your working copy of the new *ostrich* repo!**

```sh
docker build -t ostrich .
```

## Get input data and queries

The input data and queries can be get as explained for HDT.

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

## Run the experiments

Execute in the directory containing this README, in a working copy cloned from this repository.

Note - on the server you may want to do this in a **screen** session.

```sh
# this line is optional; execute in case you don't have rwx access to the folder
sudo chown $USER:docker /mnt/datastore/data/dslab/experimental/patch/

# ingestion + querying, all bearkinds
# (in preferred order of execution)
./run-docker.sh ingest-query bearb-day  2>&1 | tee ingest-query-bearb-day.log
./run-docker.sh ingest-query bearb-hour 2>&1 | tee ingest-query-bearb-hour.log
./run-docker.sh ingest-query beara      2>&1 | tee ingest-query-beara.log
```

## Copy the data to this repo

We can copy the data on the server (donizetti) to this repo, in a subdirectory `Results/output` of the current folder
using the bash script `./get-output.sh`.
