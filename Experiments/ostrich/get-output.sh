#!/bin/bash
mkdir -p Results/output
rsync -rtvz donizetti.labnet:/mnt/datastore/data/dslab/experimental/patch/output-2021-ostrich/ Results/output
