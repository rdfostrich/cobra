#!/bin/bash

# directory ${parentdir} should exist and you should have rwx access to it. It should also contain prepared input data
parentdir=/mnt/datastore/data/dslab/experimental/patch
evalrunbasedir=${parentdir}/evalrun-2021-cobra
outputbasedir=${parentdir}/output-2021-cobra

whattodo=$1
bearkind=$2
ingestionkind=$3

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
    querydir=${parentdir}/BEAR/queries_new/split
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
    echo "Usage: $0 {ingest|query|ingest-query} {beara|bearb-day|bearb-hour} ..."
    exit 2
    ;;
esac

case "${ingestionkind}" in
  cobra_opt)
    evalrundir=${evalrunbasedir}/${bearkind}/cobra_opt
    ;;
  pre_fix_up | fix_up)
    evalrundir=${evalrunbasedir}/${bearkind}/pre_fix_up-fix_up
    ;;
  *)
    echo "Usage: $0 {ingest|query|ingest-query} {beara|bearb-day|bearb-hour} {cobra_opt|pre_fix_up|fix_up}"
    exit 2
    ;;
esac

mkdir -p ${evalrundir}
if [[ ! -d ${evalrundir} ]] ; then echo "Adapt permissions to allow creation of ${evalrundir} and come back!" ; exit 2 ; fi
outputdir=${outputbasedir}/${bearkind}/${ingestionkind}
mkdir -p ${outputdir}
if [[ ! -d ${outputdir} ]] ; then echo "Adapt permissions to allow creation of ${outputdir} and come back!" ; exit 2 ; fi

imagename=cobra
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
# 1: ingestion option ("cobra_opt")
# 2: base directory
# 3: path to prepared data for ingestion (patches)
# 4: number of patches to handle
if [[ ${whattodo} =~ ingest.* ]]; then

echo "===== Running ${imagename} docker to ingest for ${bearkind}, ${ingestionkind}"
containername=${imagename}-ingest
docker run -it --name ${containername} \
-v ${evalrundir}/:/var/evalrun \
-v ${datasetdir}/:/var/patches \
${imagename} ${ingestionkind} ./ /var/patches ${number_of_patches}
docker logs ${containername} > ${outputdir}/ingest-output.txt 2> ${outputdir}/ingest-stderr.txt
docker rm ${containername}
mv ${evalrundir}/${ingestionkind}.txt ${outputdir}/

fi

# query
#
# arguments in docker run command after ${imagename}:
# 1: query option ("query")
# 2: base directory
# 3: path to query file
# 4: number of patches to handle
# 5: number of replications
# 6: number of lines to skip in the query file
if [[ ${whattodo} =~ .*query ]]; then

for query in ${queries[@]}; do

echo "===== Running ${imagename} docker for ${bearkind}, ${ingestionkind}, ${query} "
containername=${imagename}-query
docker run --name ${containername} -it \
-v ${evalrundir}/:/var/evalrun \
-v ${querydir}/:/var/queries \
${imagename} query ./ /var/queries/${query} ${number_of_patches} ${replications} 0
mv ${evalrundir}/query.txt ${outputdir}/${query}.txt
docker logs ${containername} > ${outputdir}/${query}.overall.txt 2> ${outputdir}/${query}.stderr.txt
docker rm ${containername}

done

fi
