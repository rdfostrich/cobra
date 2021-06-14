#!/bin/bash

# directory ${parentdir} should exist and you should have rwx access to it. It should also contain prepared input data
parentdir=/mnt/datastore/data/dslab/experimental/patch
evalrunbasedir=${parentdir}/evalrun-2021-cobra
outputbasedir=${parentdir}/output-2021-cobra

case "$1" in
  beara)
    datasetdir=${parentdir}/data
    querydir=${parentdir}/BEAR/queries_new
    number_of_patches=10
    ;;
  bearb-day)
    datasetdir=${parentdir}/rawdata-bearb/patches-day
    querydir=${parentdir}/BEAR/queries_bearb
    number_of_patches=89
    ;;
  bearb-hour)
    datasetdir=${parentdir}/rawdata-bearb/patches-hour
    querydir=${parentdir}/BEAR/queries_bearb
    number_of_patches=1299
    ;;
  *)
    echo "Usage: $0 {beara|bearb-day|bearb-hour}"
    exit 2
    ;;
esac

evalrundir=${evalrunbasedir}/$1
mkdir -p ${evalrundir}
if [[ ! -d ${evalrundir} ]] ; then echo "Adapt permissions to allow creation of ${evalrundir} and come back!" ; exit 2 ; fi
outputdir=${outputbasedir}/$1
mkdir -p ${outputdir}
if [[ ! -d ${outputdir} ]] ; then echo "Adapt permissions to allow creation of ${outputdir} and come back!" ; exit 2 ; fi

imagename=cobra
replications=5

queries=$(cd ${querydir} && ls -v)
echo ${queries}

# Overrides for local testing - to be put in comments in committed version
#replications=1
#case "$1" in
#  beara)
#    queries="po-queries-lowCardinality.txt"
#    ;;
#  bearb-day | bearb-hour)
#    queries="p.txt"
#    ;;
#esac
# End overrides for local testing


# ingest
#
# arguments in docker run command after ${imagename}:
# 1: ingestion option ("cobra_opt")
# 2: base directory
# 3: path to prepared data for ingestion (patches)
# 4: number of patches to handle
echo "===== Running ${imagename} docker to ingest for $1"
containername=${imagename}-ingest
docker run -it --name ${containername} \
-v ${evalrundir}/:/var/evalrun \
-v ${datasetdir}/:/var/patches \
${imagename} cobra_opt ./ /var/patches ${number_of_patches}
docker logs ${containername} > ${outputdir}/ingest-output.txt 2> ${outputdir}/ingest-stderr.txt
docker rm ${containername}

# query
#
# arguments in docker run command after ${imagename}:
# 1: query option ("query")
# 2: base directory
# 3: path to query file
# 4: number of patches to handle
# 5: number of replications
# 6: number of lines to skip in the query file
for query in ${queries[@]}; do

echo "===== Running ${imagename} docker for $1, ${query} "
docker run --rm -it \
-v ${evalrundir}/:/var/evalrun \
-v ${querydir}/:/var/queries \
${imagename} query ./ /var/queries/${query} ${number_of_patches} ${replications} 0 > ${outputdir}/${query}.overall.txt
mv ${evalrundir}/query.txt ${outputdir}/${query}.txt

done

