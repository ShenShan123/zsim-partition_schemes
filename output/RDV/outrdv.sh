#!/bin/bash
benchname=$1;
assoc=$2;
size=$3

if [[ -z "$benchname" ]]
then 
	echo "no benchmark specified!!"
	exit
fi

for dir in ./*
do
	if [[ ${dir} =~ ${benchname} ]]
	then
		echo "$dir"
		cd ${dir}
		cp ../README.stats ./
		python README.stats
		cd ../
	fi
done
