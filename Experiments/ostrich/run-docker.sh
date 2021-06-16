#!/bin/bash

# directory ${parentdir} should exist and you should have rwx access to it. It should also contain prepared input data
parentdir=/mnt/datastore/data/dslab/experimental/patch
evalrunbasedir=${parentdir}/evalrun-2021-ostrich
outputbasedir=${parentdir}/output-2021-ostrich

whattodo=$1
bearkind=$2

case "${whattodo}" in
  ingest | query | ingest-query)
    ;;
  *)
    echo "Usage: $0 {ingest|query|ingest-query} ..."
    exit 2
    ;;
esac

case "${bearkind}" in
  beara)
    datasetdir=${parentdir}/data
    querydir=${parentdir}/BEAR/queries_new
    patch_id_start=1
    patch_id_end=10
    ;;
  bearb-day)
    datasetdir=${parentdir}/rawdata-bearb/patches-day
    querydir=${parentdir}/BEAR/queries_bearb
    patch_id_start=1
    patch_id_end=89
    ;;
  bearb-hour)
    datasetdir=${parentdir}/rawdata-bearb/patches-hour
    querydir=${parentdir}/BEAR/queries_bearb
    patch_id_start=1
    patch_id_end=1299
    ;;
  *)
    echo "Usage: $0 {ingest|query|ingest-query} {beara|bearb-day|bearb-hour}"
    exit 2
    ;;
esac

evalrundir=${evalrunbasedir}/${bearkind}
mkdir -p ${evalrundir}
if [[ ! -d ${evalrundir} ]] ; then echo "Adapt permissions to allow creation of ${evalrundir} and come back!" ; exit 2 ; fi
outputdir=${outputbasedir}/${bearkind}
mkdir -p ${outputdir}
if [[ ! -d ${outputdir} ]] ; then echo "Adapt permissions to allow creation of ${outputdir} and come back!" ; exit 2 ; fi

imagename=ostrich
replications=5

queries=$(cd ${querydir} && ls -v)
echo ${queries}

# Overrides for local testing - to be put in comments in committed version
#replications=1
#case "${bearkind}" in
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
# 1: path to prepared data for ingestion (patches)
# 2: number of first patch to handle
# 3: number of last patch to handle
if [[ ${whattodo} =~ ingest.* ]]; then

echo "===== Running ${imagename} docker to ingest for ${bearkind}"
containername=${imagename}-ingest
docker run -it --name ${containername} \
-v ${evalrundir}/:/var/evalrun \
-v ${datasetdir}/:/var/patches \
${imagename} /var/patches ${patch_id_start} ${patch_id_end}
docker logs ${containername} > ${outputdir}/ingest-output.txt 2> ${outputdir}/ingest-stderr.txt
docker rm ${containername}

fi

# query
#
# arguments in docker run command after ${imagename}:
# 1: path to prepared data for ingestion (patches)
# 2: number of first patch to handle
# 3: number of last patch to handle
# 4: path to query file
# 5: number of replications
if [[ ${whattodo} =~ .*query ]]; then

for query in ${queries[@]}; do

echo "===== Running ${imagename} docker for ${bearkind}, ${query} "
docker run --rm -it \
-v ${evalrundir}/:/var/evalrun \
-v ${datasetdir}/:/var/patches \
-v ${querydir}/:/var/queries \
${imagename} /var/patches ${patch_id_start} ${patch_id_end} /var/queries/${query} ${replications} > ${outputdir}/${query}.txt

done

fi
