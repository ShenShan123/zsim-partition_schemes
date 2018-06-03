#!/bin/bash
benchname=$1;
size=$2;

if [[ -z "$benchname" || -z "$size" ]]
then 
	echo "no benchmark or LLC size specified!!"
	exit
fi

#echo ${benchname}-${size};

for ((i = 2; i <= 64; i = i * 2))
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
		echo ${benchname}
		sed -i "s/command = abc/command = \"\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/perlbench_r_base.mytest-64 \-I\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/lib \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/diffmail.pl 4 800 10 17 19 300\"/" pgo.cfg
	elif [[ $benchname =~ 505.mcf_r ]]
	then
		sed -i "s/command = abc/command = \"\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/mcf_r_base.mytest-64 \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/inp.in\"/" pgo.cfg
	elif [[ $benchname =~ 520.omnetpp_r ]]
	then
		sed -i "s/command = abc/command = \"\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/omnetpp_r_base.mytest-64 \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/omnetpp.ini\"/" pgo.cfg
	elif [[ $benchname =~ 531.deepsjeng_r ]]
	then
		sed -i "s/command = abc/command = \"\/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/deepsjeng_r_base.mytest-64 \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/ref.txt\"/" pgo.cfg
	elif [[ $benchname =~ 538.imagick_r ]]
	then
		sed -i "s/command = abc/command = \"convert -limit disk 0 \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/refrate_input.tga -edge 41 -resample 181% -emboss 31 -colorspace YUV 19x19+15% -resize 30% \/mnt\/hgfs\/ShareShen\/zsim-master\/output\/RDV\/${benchname}\/refrate_output.tga\"/" pgo.cfg
	fi
	sed -i "s/ways = [0-9]*; \/\/ l3 assoc/ways = ${i}; \/\/ l3 assoc/" pgo.cfg
	#sed -i "s/size = [0-9]*; \/\/ l3 size/size = ${i}; \/\/ l3 size/" pgo.cfg
	echo "set the size of L3 to ${i}"
	./../../../build/opt/zsim ./pgo.cfg
	cd ../
	
	
	if [ ! -d $benchname-${assoc}-${size} ]
	then
		mkdir $benchname-${assoc}-${size}
	fi
	
	cp $benchname/*.h5 $benchname/*.out $benchname/*.cfg $benchname-${assoc}-${size}
done

#sed -i "s/command = .*/c'command = abc'/" pgo.cfg

