#!/bin/bash
# A utility to split files in a directory into single line files in a different directory
#
# Parameters
#   $1: input directory (default: .)
#   $2: output directory (will be created) (default: $1/split)
#   $3: input file selection pattern (default: *.txt)

INDIR=${1:-.}
OUTDIR=${2:-${INDIR}/split}
PATTERN=${3:-*.txt}

if [[ ! -d ${INDIR} ]] ; then echo "Input directory ${INDIR} does not exist!" ; exit 2 ; fi
mkdir -p ${OUTDIR}
if [[ ! -d ${OUTDIR} ]] ; then echo "Adapt permissions to allow creation of ${OUTDIR} and come back!" ; exit 2 ; fi

find ${INDIR} -maxdepth 1 -type f -name "${PATTERN}" | while read f ; do
  echo $f
  base=${f##*/}
  split -l 1 -a 4 -d $f ${OUTDIR}/${base}.
done
