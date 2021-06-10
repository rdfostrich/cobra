#!/bin/bash

# directory 'basedir' should exist and you should have rwx access to it
basedir=/mnt/datastore/data/dslab/experimental/patch/
writedir=${basedir}cobra-2021/

case "$1" in
  beara)
    datasetdir=${basedir}data/
    querydir=${basedir}BEAR/queries_new/
    number_of_patches=10
    ;;
  bearb-day)
    datasetdir=${basedir}rawdata-bearb/patches-day/
    querydir=${basedir}BEAR/queries_bearb/
    number_of_patches=89
    ;;
  bearb-hour)
    datasetdir=${basedir}rawdata-bearb/patches-hour/
    querydir=${basedir}BEAR/queries_bearb/
    number_of_patches=1299
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
echo "===== Running ${imagename} docker to ingest for $1"
containername=${imagename}-ingest
docker run -it --name ${containername} \
-v ${evalrundir}:/var/evalrun \
-v ${datasetdir}:/var/patches \
${imagename} cobra_opt ./ /var/patches ${number_of_patches}
docker logs ${containername} > ${outputdir}ingest-output.txt 2> ${outputdir}ingest-stderr.txt
docker rm ${containername}

# query
for query in ${queries[@]}; do

echo "===== Running ${imagename} docker for $1, ${query} "
docker run --rm -it \
-v ${evalrundir}:/var/evalrun \
-v ${datasetdir}:/var/patches \
-v ${querydir}:/var/queries \
${imagename} query ./ /var/patches ${number_of_patches} /var/queries/${query} ${replications} 0 > ${outputdir}/$query.txt

done

