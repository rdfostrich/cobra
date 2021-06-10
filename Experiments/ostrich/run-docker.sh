#!/bin/bash

# directory 'basedir' should exist and you should have rwx access to it
basedir=/mnt/datastore/data/dslab/experimental/patch/
writedir=${basedir}ostrich-2021/

case "$1" in
  beara)
    datasetdir=${basedir}data/
    querydir=${basedir}BEAR/queries_new/
    patch_id_start=1
    patch_id_end=10
    ;;
  bearb-day)
    datasetdir=${basedir}rawdata-bearb/patches-day/
    querydir=${basedir}BEAR/queries_bearb/
    patch_id_start=1
    patch_id_end=89
    ;;
  bearb-hour)
    datasetdir=${basedir}rawdata-bearb/patches-hour/
    querydir=${basedir}BEAR/queries_bearb/
    patch_id_start=1
    patch_id_end=1299
    ;;
  *)
    echo "Usage: $0 {beara|bearb-day|bearb-hour}"
    exit 2
    ;;
esac

evalrundir=${writedir}evalrun-$1/
mkdir -p ${evalrundir}
if [[ ! -d ${evalrundir} ]] ; then echo "Adapt permissions to allow creation of ${evalrundir} and come back!" ; exit 2 ; fi
outputdir=${writedir}output-$1/
mkdir -p ${outputdir}
if [[ ! -d ${outputdir} ]] ; then echo "Adapt permissions to allow creation of ${outputdir} and come back!" ; exit 2 ; fi

imagename=ostrich
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
echo "===== Running ${imagename} docker to ingest for $1"
containername=${imagename}-ingest
docker run -it --name ${containername} \
-v ${evalrundir}:/var/evalrun \
-v ${datasetdir}:/var/patches \
${imagename} /var/patches ${patch_id_start} ${patch_id_end}
docker logs ${containername} > ${outputdir}ingest-output.txt 2> ${outputdir}ingest-stderr.txt
docker rm ${containername}

# query
for query in ${queries[@]}; do

echo "===== Running ${imagename} docker for $1, ${query} "
docker run --rm -it \
-v ${evalrundir}:/var/evalrun \
-v ${datasetdir}:/var/patches \
-v ${querydir}:/var/queries \
${imagename} /var/patches ${patch_id_start} ${patch_id_end} /var/queries/${query} ${replications} > ${outputdir}/$query.txt

done

