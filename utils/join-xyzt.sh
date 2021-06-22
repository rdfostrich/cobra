#!/bin/bash
# A utility to join files in a directory that have a '*.txt.????.txt' pattern in their name.
# The joined output is in files named '*.txt.txt', i.e. the '.????' portion is deleted
#
# Parameters
#   $1: input directory (default: .)
#   $2: output directory (default: $1)

INDIR=${1:-.}
OUTDIR=${2:-${INDIR}}

if [[ ! -d ${INDIR} ]] ; then echo "Input directory ${INDIR} does not exist!" ; exit 2 ; fi
mkdir -p ${OUTDIR}
if [[ ! -d ${OUTDIR} ]] ; then echo "Adapt permissions to allow creation of ${OUTDIR} and come back!" ; exit 2 ; fi

find ${INDIR} -maxdepth 1 -type f -iname '*.txt.????.txt' | sort | while read f ; do
  echo $f
  base=${f##*/}
  outfile=$(echo $base | sed 's/\(.*\)\.....\(.*\)/\1\2/g')
  cat $f >> ${OUTDIR}/${outfile}
done
