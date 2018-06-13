#!/bin/bash
benchname=$1;
size=$2;
way=$3;

if [[ -z "$benchname" || -z "$size" || -z "$way" ]]
then 
	echo "no benchmark or LLC size or associativity specified!!"
	exit
fi

#echo ${benchname}-${size};

for ((i = 32; i >= 2; i = i / 2))
do
	if [ ! -d ${benchname} ]
	then
		mkdir ${benchname}
	fi

	cd ${benchname}
	echo $PWD
	cp ../pgo.cfg ./
	cp -u ../../../../spec06/${benchname}/* ./ -r
	echo "done copy ${benchname} directory"
	# specify the command line
	command=""
	if [[ $benchname =~ 500.perlbench_r ]]
	then
		command="command = \"\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/perlbench_r_base.mytest-64 \-I\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/lib \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/diffmail.pl 4 800 10 17 19 300\""
	elif [[ $benchname =~ 505.mcf_r ]]
	then
		command="command = \"\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/mcf_r_base.mytest-64 \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/inp.in\""
	elif [[ $benchname =~ 507.cactuBSSN_r ]]
	then
		command="command = \"\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/cactuBSSN_r_base.mytest-64 \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/spec_ref.par\""
	elif [[ $benchname =~ 520.omnetpp_r ]]
	then
		command="command = \"\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/omnetpp_r_base.mytest-64 \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/omnetpp.ini\""
	elif [[ $benchname =~ 531.deepsjeng_r ]]
	then
		command="command = \"\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/deepsjeng_r_base.mytest-64 \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/ref.txt\""
	elif [[ $benchname =~ 538.imagick_r ]]
	then
		command="command = \"convert -limit disk 0 \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/refrate_input.tga -edge 41 -resample 181% -emboss 31 -colorspace YUV 19x19+15% -resize 30% \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/refrate_output.tga\""
	elif [[ $benchname =~ ^sjeng ]]
	then
		command="command = \"\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/sjeng_base.gcc41-amd64bit \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/ref.txt\""
	elif [[ $benchname =~ ^bwaves ]]
	then
		command="command = \"\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/bwaves_base.gcc41-amd64bit\""
	fi
	# adding the processes
	for (( k = 0; k < i; k++))
	do
		line=$((4 + 3 * k))
		sed -i "${line} a\process${k} = \{\n\tcommand = abc\n\}\;" ./pgo.cfg
	done
	sed -i "s/command = abc/${command}/" pgo.cfg
	
	j=0
	if [ "$size" = "8K" ]
	then
		j=8192
	elif [ "$size" = "16K" ]
	then
		j=16384
	elif [ "$size" = "32K" ]
	then
		j=32768
	elif [ "$size" = "64K" ]
	then
		j=65536
	elif [ "$size" = "128K" ]
	then
		j=131072
	elif [ "$size" = "256K" ]
	then
		j=262144
	elif [ "$size" = "512K" ]
	then
		j=524288
	elif [ "$size" = "1M" ]
	then 
		j=1048576
	elif [ "$size" = "2M" ]
	then
		j=2097152
	elif [ "$size" = "4M" ]
	then
		j=4194304
	elif [ "$size" = "8M" ]
	then
		j=8388608
	fi
	
	# change the LLC config
	sed -i "s/ways = [0-9]*; \/\/ l3 assoc/ways = ${way}; \/\/ l3 assoc/" pgo.cfg
	sed -i "s/size = [0-9]*; \/\/ l3 size/size = ${j}; \/\/ l3 size/" pgo.cfg
	sed -i "s/caches = [0-9]*; \/\/ L1 caches/caches = ${i}; \/\/ L1 caches/" pgo.cfg
	sed -i "s/caches = [0-9]*; \/\/ L2 caches/caches = ${i}; \/\/ L2 caches/" pgo.cfg
	sed -i "s/cores = [0-9]*;/cores = ${i}/" pgo.cfg
	#sed -i "s/banks = [0-9]*; \/\/ L3 banks/banks = ${i}; \/\/ L3 banks" pgo.cfg
	echo "num of cores ${i}, # of private caches ${i}, L3 size ${size} assoc ${way}"
	
	# start the simulation
	./../../../build/opt/zsim ./pgo.cfg
	cd ../
	
	
	if [ ! -d $benchname-c${i}-${way}-${size} ]
	then
		mkdir $benchname-c${i}-${way}-${size}
	fi
	
	cp $benchname/*.h5 $benchname/*.out $benchname/*.cfg $benchname-c${i}-${way}-${size}
done