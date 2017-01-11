#!/bin/bash
#grademerge in=<infile> out=<outfile>

function usage(){
echo "
Written by Brian Bushnell
Last modified February 17, 2015

Description:  Grades correctness of merging synthetic reads with headers 
              generated by RandomReads and re-headered by RenameReads.

Usage:  grademerge.sh in=<file>

Parameters:
in=<file>           Specify the input file, or 'stdin'.

Please contact Brian Bushnell at bbushnell@lbl.gov if you encounter any problems.
"
}

pushd . > /dev/null
DIR="${BASH_SOURCE[0]}"
while [ -h "$DIR" ]; do
  cd "$(dirname "$DIR")"
  DIR="$(readlink "$(basename "$DIR")")"
done
cd "$(dirname "$DIR")"
DIR="$(pwd)/"
popd > /dev/null

#DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/"
CP="$DIR""current/"
EA="-ea"
set=0

if [ -z "$1" ] || [[ $1 == -h ]] || [[ $1 == --help ]]; then
	usage
	exit
fi

function grademerge() {
	if [[ $NERSC_HOST == genepool ]]; then
		module unload oracle-jdk
		module load oracle-jdk/1.8_64bit
		module load pigz
	fi
	local CMD="java $EA -Xmx200m -cp $CP jgi.GradeMergedReads $@"
#	echo $CMD >&2
	eval $CMD
}

grademerge "$@"
