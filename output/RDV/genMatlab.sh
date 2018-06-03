#!/bin/bash
echo "rdvs = zeros(54,512);"
echo "hits = zeros(54,512);"
echo "rfunc = zeros(54,512);"
echo "afunc = zeros(54,512);"
echo "hitrate = zeros(54,1);"

k=0

for (( i = 2; i <= 64; i = i * 2 ))
do
	for (( j = 8192; j <= 2097152; j = j * 2 ))
	do
		size=''
		k=$((k+1))
		if [ $j -eq 8192 ]
		then
			size='8K'
		elif [ $j -eq 16384 ]
		then
			size='16K'
		elif [ $j -eq 32768 ]
		then
			size='32K'
		elif [ $j -eq 65536 ]
		then
			size='64K'
		elif [ $j -eq 131072 ]
		then
			size='128K'
		elif [ $j -eq 262144 ]
		then
			size='256K'
		elif [ $j -eq 524288 ]
		then
			size='512K'
		elif [ $j -eq 1048576 ]
		then 
			size='1M'
		elif [ $j -eq 2097152 ]
		then
			size='2M'
		fi
		echo "load 505.mcf_r-${i}-${size}/rdd.txt"
		echo "load 505.mcf_r-${i}-${size}/miss.txt"
		echo "rdvs(${k},:) = rdd(1,:);"
		echo "hits(${k},:) = rdd(2,:);"
		echo "hitrate(${k}) = miss(1) / miss(2);"
		echo "rfunc(${k},:) = rdd(2,:) ./ rdd(1,:);"
		echo "afunc(${k},:) = rdd(4,:) ./ rdd(3,:);"
	done
done
