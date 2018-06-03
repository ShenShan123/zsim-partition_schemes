#!/bin/bash
benchname=$1;
assoc=$2;

if [[ -z "$benchname" || -z "$assoc" ]]
then 
	echo "no benchmark or LLC assoc specified!!"
	exit
fi

#echo ${benchname}-${assoc};
j=$2 # this is the associativity
for ((j = $2; j <= 64; j = j * 2))
do
for ((i = 8192; i <= 2097152; i = i * 2))
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
	
	if [[ $benchname =~ hmmer ]]
	then
		sed -i "s/command = abc/command = \"\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/hmmer_base.gcc41-amd64bit \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/nph3\.hmm \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/swiss41\"/" pgo.cfg
	elif [[ $benchname =~ bzip2 ]]
	then
		sed -i "s/command = abc/command = \"\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/bzip2_base.gcc41-amd64bit \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/chicken.jpg 30\"/" pgo.cfg
	elif [[ $benchname =~ ^mcf$ ]]
	then
		sed -i "s/command = abc/command = \"\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/mcf_base.gcc41-amd64bit \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/inp.in\"/" pgo.cfg
	elif [[ $benchname =~ ^omnetpp$ ]]
	then
		sed -i "s/command = abc/command = \"\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/omnetpp_base.gcc41-amd64bit \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/omnetpp.ini\"/" pgo.cfg
	elif [[ $benchname =~ fotonik3d ]]
	then
		sed -i "s/command = abc/command = \"\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/fotonik3d_r_base.mytest-64 \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/yee.dat\"/" pgo.cfg
	elif [[ $benchname =~ bwaves ]]
	then
		sed -i "s/command = abc/command = \"\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/bwaves_base.gcc41-amd64bit\"/" pgo.cfg
	elif [[ $benchname =~ 500.perlbench_r ]]
	then
		sed -i "s/command = abc/command = \"\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/perlbench_r_base.mytest-64 \-I\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/lib \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/diffmail.pl 4 800 10 17 19 300\"/" pgo.cfg
	elif [[ $benchname =~ 505.mcf_r ]]
	then
		sed -i "s/command = abc/command = \"\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/mcf_r_base.mytest-64 \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/inp.in\"/" pgo.cfg
	elif [[ $benchname =~ 507.cactuBSSN_r ]]
	then
		sed -i "s/command = abc/command = \"\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/cactuBSSN_r_base.mytest-64 \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/spec_ref.par\"/" pgo.cfg
	elif [[ $benchname =~ 520.omnetpp_r ]]
	then
		sed -i "s/command = abc/command = \"\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/omnetpp_r_base.mytest-64 \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/omnetpp.ini\"/" pgo.cfg
	elif [[ $benchname =~ 531.deepsjeng_r ]]
	then
		sed -i "s/command = abc/command = \"\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/deepsjeng_r_base.mytest-64 \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/ref.txt\"/" pgo.cfg
	elif [[ $benchname =~ 538.imagick_r ]]
	then
		sed -i "s/command = abc/command = \"convert -limit disk 0 \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/refrate_input.tga -edge 41 -resample 181% -emboss 31 -colorspace YUV 19x19+15% -reassoc 30% \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/refrate_output.tga\"/" pgo.cfg
	fi
	sed -i "s/ways = [0-9]*; \/\/ l3 assoc/ways = ${j}; \/\/ l3 assoc/" pgo.cfg
	sed -i "s/size = [0-9]*; \/\/ l3 size/size = ${i}; \/\/ l3 size/" pgo.cfg
	echo "set the assoc of L3 to ${j}, size ${i}"
	
	./../../../build/opt/zsim ./pgo.cfg
	cd ../
		
	size=''
	if [ $i -eq 8192 ]
	then
		size='8K'
	elif [ $i -eq 16384 ]
	then
		size='16K'
	elif [ $i -eq 32768 ]
	then
		size='32K'
	elif [ $i -eq 65536 ]
	then
		size='64K'
	elif [ $i -eq 131072 ]
	then
		size='128K'
	elif [ $i -eq 262144 ]
	then
		size='256K'
	elif [ $i -eq 524288 ]
	then
		size='512K'
	elif [ $i -eq 1048576 ]
	then 
		size='1M'
	elif [ $i -eq 2097152 ]
	then
		size='2M'
	fi
	if [ ! -d "$benchname-${j}-${size}" ]
	then
		mkdir $benchname-${j}-${size}
	fi
	
	cp $benchname/*.h5 $benchname/*.out $benchname/*.cfg $benchname-${j}-${size}
	echo "simualtion done on $benchname-${j}-${size}"
	
	#j=$((j*2))
done
done

