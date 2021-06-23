#!/bin/bash
# A utility to join files in a directory that have a '*.txt.????.*' pattern in their name.
# The joined output is in files named '*.txt.*', i.e. the '.????' portion is deleted.
# (The ???? can be 4 digits or 4 characters; output is joined in alphabetical order of this portion).
#
# Parameters
#   $1: input directory (default: .)
#   $2: output directory (default: $1/joined)

INDIR=${1:-.}
OUTDIR=${2:-${INDIR}/joined}

if [[ ! -d ${INDIR} ]] ; then echo "Input directory ${INDIR} does not exist!" ; exit 2 ; fi
mkdir -p ${OUTDIR}
if [[ ! -d ${OUTDIR} ]] ; then echo "Adapt permissions to allow creation of ${OUTDIR} and come back!" ; exit 2 ; fi

find ${INDIR} -maxdepth 1 -type f -iname '*.txt.????.*' | sort | while read f ; do
  echo $f
  base=${f##*/}
  outfile=$(echo $base | sed 's/\(.*\)\.txt\.....\.\(.*\)/\1.txt.\2/g')
  cat $f >> ${OUTDIR}/${outfile}
done
