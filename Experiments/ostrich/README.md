# Reproducing OSTRICH experiments

This documentation is as good as identical to the documentation for the COBRA experiment.
The few differences are marked with !!DIFF!! in this documentation.

Execute all commands shown below in this directory of a working copy cloned from this repository
(unless explicitly stated otherwise).

## Prerequisites

### Docker

Have [docker](https://docs.docker.com/get-docker/) installed and [get access as non-root user](https://docs.docker.com/engine/install/linux-postinstall/#manage-docker-as-a-non-root-user).

### Additional repo (!!DIFF!!)

The code for Ostrich is in a separate git repository!

Clone that repo with the `--recurse-submodules` option in your `git clone` command.
**Do this in a parent directory outside your working copy of this *cobra* repo!**
```
git clone https://github.com/rdfostrich/ostrich.git --recurse-submodules
```

## Create docker image (!!DIFF!!)

**Do this in the root directory of your working copy of the new *ostrich* repo!**
```sh
docker build -t ostrich .
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

Note - on the server you may want to do this in a **screen** session.
```sh
# this line is optional; execute in case you don't have rwx access to the folder
sudo chown $USER:docker /mnt/datastore/data/dslab/experimental/patch/

./run-docker.sh beara 2>&1 | tee beara.log
./run-docker.sh bearb-day 2>&1 | tee bearb-day.log
./run-docker.sh bearb-hour 2>&1 | tee bearb-hour.log
```
