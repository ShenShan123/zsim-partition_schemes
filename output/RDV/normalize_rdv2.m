
rdvs = zeros(54,512);
age = zeros(54,512);
hits = zeros(54,512);
rfunc = zeros(54,512);
afunc = zeros(54,512);
hitrate = zeros(54,1);
load 505.mcf_r-2-8K/rdd.txt
load 505.mcf_r-2-8K/miss.txt
rdvs(1,:) = rdd(1,:);
hits(1,:) = rdd(2,:);
hitrate(1) = miss(1) / miss(2);
rfunc(1,:) = rdd(2,:) ./ rdd(1,:);
ageHits(1,:) = rdd(4,:);
afunc(1,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-2-16K/rdd.txt
load 505.mcf_r-2-16K/miss.txt
rdvs(2,:) = rdd(1,:);
hits(2,:) = rdd(2,:);
hitrate(2) = miss(1) / miss(2);
rfunc(2,:) = rdd(2,:) ./ rdd(1,:);
ageHits(2,:) = rdd(4,:);
afunc(2,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-2-32K/rdd.txt
load 505.mcf_r-2-32K/miss.txt
rdvs(3,:) = rdd(1,:);
hits(3,:) = rdd(2,:);
hitrate(3) = miss(1) / miss(2);
rfunc(3,:) = rdd(2,:) ./ rdd(1,:);
ageHits(3,:) = rdd(4,:);
afunc(3,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-2-64K/rdd.txt
load 505.mcf_r-2-64K/miss.txt
rdvs(4,:) = rdd(1,:);
hits(4,:) = rdd(2,:);
hitrate(4) = miss(1) / miss(2);
rfunc(4,:) = rdd(2,:) ./ rdd(1,:);
ageHits(4,:) = rdd(4,:);
afunc(4,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-2-128K/rdd.txt
load 505.mcf_r-2-128K/miss.txt
rdvs(5,:) = rdd(1,:);
hits(5,:) = rdd(2,:);
hitrate(5) = miss(1) / miss(2);
rfunc(5,:) = rdd(2,:) ./ rdd(1,:);
ageHits(5,:) = rdd(4,:);
afunc(5,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-2-256K/rdd.txt
load 505.mcf_r-2-256K/miss.txt
rdvs(6,:) = rdd(1,:);
hits(6,:) = rdd(2,:);
hitrate(6) = miss(1) / miss(2);
rfunc(6,:) = rdd(2,:) ./ rdd(1,:);
ageHits(6,:) = rdd(4,:);
afunc(6,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-2-512K/rdd.txt
load 505.mcf_r-2-512K/miss.txt
rdvs(7,:) = rdd(1,:);
hits(7,:) = rdd(2,:);
hitrate(7) = miss(1) / miss(2);
rfunc(7,:) = rdd(2,:) ./ rdd(1,:);
ageHits(7,:) = rdd(4,:);
afunc(7,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-2-1M/rdd.txt
load 505.mcf_r-2-1M/miss.txt
rdvs(8,:) = rdd(1,:);
hits(8,:) = rdd(2,:);
hitrate(8) = miss(1) / miss(2);
rfunc(8,:) = rdd(2,:) ./ rdd(1,:);
ageHits(8,:) = rdd(4,:);
afunc(8,:) = rdd(4,:) ./ rdd(3,:);

load 505.mcf_r-2-2M/rdd.txt
load 505.mcf_r-2-2M/miss.txt
rdvs(9,:) = rdd(1,:);
hits(9,:) = rdd(2,:);
hitrate(9) = miss(1) / miss(2);
rfunc(9,:) = rdd(2,:) ./ rdd(1,:);
ageHits(9,:) = rdd(4,:);
afunc(9,:) = rdd(4,:) ./ rdd(3,:);

load 505.mcf_r-4-8K/rdd.txt
load 505.mcf_r-4-8K/miss.txt
rdvs(10,:) = rdd(1,:);
hits(10,:) = rdd(2,:);
hitrate(10) = miss(1) / miss(2);
rfunc(10,:) = rdd(2,:) ./ rdd(1,:);
ageHits(10,:) = rdd(4,:);
afunc(10,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-4-16K/rdd.txt
load 505.mcf_r-4-16K/miss.txt
rdvs(11,:) = rdd(1,:);
hits(11,:) = rdd(2,:);
hitrate(11) = miss(1) / miss(2);
rfunc(11,:) = rdd(2,:) ./ rdd(1,:);
ageHits(11,:) = rdd(4,:);
afunc(11,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-4-32K/rdd.txt
load 505.mcf_r-4-32K/miss.txt
rdvs(12,:) = rdd(1,:);
hits(12,:) = rdd(2,:);
hitrate(12) = miss(1) / miss(2);
rfunc(12,:) = rdd(2,:) ./ rdd(1,:);
ageHits(12,:) = rdd(4,:);
afunc(12,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-4-64K/rdd.txt
load 505.mcf_r-4-64K/miss.txt
rdvs(13,:) = rdd(1,:);
hits(13,:) = rdd(2,:);
hitrate(13) = miss(1) / miss(2);
rfunc(13,:) = rdd(2,:) ./ rdd(1,:);
ageHits(13,:) = rdd(4,:);
afunc(13,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-4-128K/rdd.txt
load 505.mcf_r-4-128K/miss.txt
rdvs(14,:) = rdd(1,:);
hits(14,:) = rdd(2,:);
hitrate(14) = miss(1) / miss(2);
rfunc(14,:) = rdd(2,:) ./ rdd(1,:);
ageHits(14,:) = rdd(4,:);
afunc(14,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-4-256K/rdd.txt
load 505.mcf_r-4-256K/miss.txt
rdvs(15,:) = rdd(1,:);
hits(15,:) = rdd(2,:);
hitrate(15) = miss(1) / miss(2);
rfunc(15,:) = rdd(2,:) ./ rdd(1,:);
ageHits(15,:) = rdd(4,:);
afunc(15,:) = rdd(4,:) ./ rdd(3,:);

load 505.mcf_r-4-512K/rdd.txt
load 505.mcf_r-4-512K/miss.txt
rdvs(16,:) = rdd(1,:);
hits(16,:) = rdd(2,:);
hitrate(16) = miss(1) / miss(2);
rfunc(16,:) = rdd(2,:) ./ rdd(1,:);
ageHits(16,:) = rdd(4,:);
afunc(16,:) = rdd(4,:) ./ rdd(3,:);

load 505.mcf_r-4-1M/rdd.txt
load 505.mcf_r-4-1M/miss.txt
rdvs(17,:) = rdd(1,:);
hits(17,:) = rdd(2,:);
hitrate(17) = miss(1) / miss(2);
rfunc(17,:) = rdd(2,:) ./ rdd(1,:);
ageHits(17,:) = rdd(4,:);
afunc(17,:) = rdd(4,:) ./ rdd(3,:);

load 505.mcf_r-4-2M/rdd.txt
load 505.mcf_r-4-2M/miss.txt
rdvs(18,:) = rdd(1,:);
hits(18,:) = rdd(2,:);
hitrate(18) = miss(1) / miss(2);
rfunc(18,:) = rdd(2,:) ./ rdd(1,:);
ageHits(18,:) = rdd(4,:);
afunc(18,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-8-8K/rdd.txt
load 505.mcf_r-8-8K/miss.txt
rdvs(19,:) = rdd(1,:);
hits(19,:) = rdd(2,:);
hitrate(19) = miss(1) / miss(2);
rfunc(19,:) = rdd(2,:) ./ rdd(1,:);
ageHits(19,:) = rdd(4,:);
afunc(19,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-8-16K/rdd.txt
load 505.mcf_r-8-16K/miss.txt
rdvs(20,:) = rdd(1,:);
hits(20,:) = rdd(2,:);
hitrate(20) = miss(1) / miss(2);
rfunc(20,:) = rdd(2,:) ./ rdd(1,:);
ageHits(20,:) = rdd(4,:);
afunc(20,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-8-32K/rdd.txt
load 505.mcf_r-8-32K/miss.txt
rdvs(21,:) = rdd(1,:);
hits(21,:) = rdd(2,:);
hitrate(21) = miss(1) / miss(2);
rfunc(21,:) = rdd(2,:) ./ rdd(1,:);
ageHits(21,:) = rdd(4,:);
afunc(21,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-8-64K/rdd.txt
load 505.mcf_r-8-64K/miss.txt
rdvs(22,:) = rdd(1,:);
hits(22,:) = rdd(2,:);
hitrate(22) = miss(1) / miss(2);
rfunc(22,:) = rdd(2,:) ./ rdd(1,:);
ageHits(22,:) = rdd(4,:);
afunc(22,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-8-128K/rdd.txt
load 505.mcf_r-8-128K/miss.txt
rdvs(23,:) = rdd(1,:);
hits(23,:) = rdd(2,:);
hitrate(23) = miss(1) / miss(2);
rfunc(23,:) = rdd(2,:) ./ rdd(1,:);
ageHits(23,:) = rdd(4,:);
afunc(23,:) = rdd(4,:) ./ rdd(3,:);

load 505.mcf_r-8-256K/rdd.txt
load 505.mcf_r-8-256K/miss.txt
rdvs(24,:) = rdd(1,:);
hits(24,:) = rdd(2,:);
hitrate(24) = miss(1) / miss(2);
rfunc(24,:) = rdd(2,:) ./ rdd(1,:);
ageHits(24,:) = rdd(4,:);
afunc(24,:) = rdd(4,:) ./ rdd(3,:);

load 505.mcf_r-8-512K/rdd.txt
load 505.mcf_r-8-512K/miss.txt
rdvs(25,:) = rdd(1,:);
hits(25,:) = rdd(2,:);
hitrate(25) = miss(1) / miss(2);
rfunc(25,:) = rdd(2,:) ./ rdd(1,:);
ageHits(25,:) = rdd(4,:);
afunc(25,:) = rdd(4,:) ./ rdd(3,:);

load 505.mcf_r-8-1M/rdd.txt
load 505.mcf_r-8-1M/miss.txt
rdvs(26,:) = rdd(1,:);
hits(26,:) = rdd(2,:);
hitrate(26) = miss(1) / miss(2);
rfunc(26,:) = rdd(2,:) ./ rdd(1,:);
ageHits(26,:) = rdd(4,:);
afunc(26,:) = rdd(4,:) ./ rdd(3,:);

load 505.mcf_r-8-2M/rdd.txt
load 505.mcf_r-8-2M/miss.txt
rdvs(27,:) = rdd(1,:);
hits(27,:) = rdd(2,:);
hitrate(27) = miss(1) / miss(2);
rfunc(27,:) = rdd(2,:) ./ rdd(1,:);
ageHits(27,:) = rdd(4,:);
afunc(27,:) = rdd(4,:) ./ rdd(3,:);

load 505.mcf_r-16-8K/rdd.txt
load 505.mcf_r-16-8K/miss.txt
rdvs(28,:) = rdd(1,:);
hits(28,:) = rdd(2,:);
hitrate(28) = miss(1) / miss(2);
rfunc(28,:) = rdd(2,:) ./ rdd(1,:);
ageHits(28,:) = rdd(4,:);
afunc(28,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-16-16K/rdd.txt
load 505.mcf_r-16-16K/miss.txt
rdvs(29,:) = rdd(1,:);
hits(29,:) = rdd(2,:);
hitrate(29) = miss(1) / miss(2);
rfunc(29,:) = rdd(2,:) ./ rdd(1,:);
ageHits(29,:) = rdd(4,:);
afunc(29,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-16-32K/rdd.txt
load 505.mcf_r-16-32K/miss.txt
rdvs(30,:) = rdd(1,:);
hits(30,:) = rdd(2,:);
hitrate(30) = miss(1) / miss(2);
rfunc(30,:) = rdd(2,:) ./ rdd(1,:);
ageHits(30,:) = rdd(4,:);
afunc(30,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-16-64K/rdd.txt
load 505.mcf_r-16-64K/miss.txt
rdvs(31,:) = rdd(1,:);
hits(31,:) = rdd(2,:);
hitrate(31) = miss(1) / miss(2);
rfunc(31,:) = rdd(2,:) ./ rdd(1,:);
ageHits(31,:) = rdd(4,:);
afunc(31,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-16-128K/rdd.txt
load 505.mcf_r-16-128K/miss.txt
rdvs(32,:) = rdd(1,:);
hits(32,:) = rdd(2,:);
hitrate(32) = miss(1) / miss(2);
rfunc(32,:) = rdd(2,:) ./ rdd(1,:);
ageHits(32,:) = rdd(4,:);
afunc(32,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-16-256K/rdd.txt
load 505.mcf_r-16-256K/miss.txt
rdvs(33,:) = rdd(1,:);
hits(33,:) = rdd(2,:);
hitrate(33) = miss(1) / miss(2);
rfunc(33,:) = rdd(2,:) ./ rdd(1,:);
ageHits(33,:) = rdd(4,:);
afunc(33,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-16-512K/rdd.txt
load 505.mcf_r-16-512K/miss.txt
rdvs(34,:) = rdd(1,:);
hits(34,:) = rdd(2,:);
hitrate(34) = miss(1) / miss(2);
rfunc(34,:) = rdd(2,:) ./ rdd(1,:);
ageHits(34,:) = rdd(4,:);
afunc(34,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-16-1M/rdd.txt
load 505.mcf_r-16-1M/miss.txt
rdvs(35,:) = rdd(1,:);
hits(35,:) = rdd(2,:);
hitrate(35) = miss(1) / miss(2);
rfunc(35,:) = rdd(2,:) ./ rdd(1,:);
ageHits(35,:) = rdd(4,:);
afunc(35,:) = rdd(4,:) ./ rdd(3,:);

load 505.mcf_r-16-2M/rdd.txt
load 505.mcf_r-16-2M/miss.txt
rdvs(36,:) = rdd(1,:);
hits(36,:) = rdd(2,:);
hitrate(36) = miss(1) / miss(2);
rfunc(36,:) = rdd(2,:) ./ rdd(1,:);
ageHits(36,:) = rdd(4,:);
afunc(36,:) = rdd(4,:) ./ rdd(3,:);

load 505.mcf_r-32-8K/rdd.txt
load 505.mcf_r-32-8K/miss.txt
rdvs(37,:) = rdd(1,:);
hits(37,:) = rdd(2,:);
hitrate(37) = miss(1) / miss(2);
rfunc(37,:) = rdd(2,:) ./ rdd(1,:);
ageHits(37,:) = rdd(4,:);
afunc(37,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-32-16K/rdd.txt
load 505.mcf_r-32-16K/miss.txt
rdvs(38,:) = rdd(1,:);
hits(38,:) = rdd(2,:);
hitrate(38) = miss(1) / miss(2);
rfunc(38,:) = rdd(2,:) ./ rdd(1,:);
ageHits(38,:) = rdd(4,:);
afunc(38,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-32-32K/rdd.txt
load 505.mcf_r-32-32K/miss.txt
rdvs(39,:) = rdd(1,:);
hits(39,:) = rdd(2,:);
hitrate(39) = miss(1) / miss(2);
rfunc(39,:) = rdd(2,:) ./ rdd(1,:);
ageHits(39,:) = rdd(4,:);
afunc(39,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-32-64K/rdd.txt
load 505.mcf_r-32-64K/miss.txt
rdvs(40,:) = rdd(1,:);
hits(40,:) = rdd(2,:);
hitrate(40) = miss(1) / miss(2);
rfunc(40,:) = rdd(2,:) ./ rdd(1,:);
ageHits(40,:) = rdd(4,:);
afunc(40,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-32-128K/rdd.txt
load 505.mcf_r-32-128K/miss.txt
rdvs(41,:) = rdd(1,:);
hits(41,:) = rdd(2,:);
hitrate(41) = miss(1) / miss(2);
rfunc(41,:) = rdd(2,:) ./ rdd(1,:);
ageHits(41,:) = rdd(4,:);
afunc(41,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-32-256K/rdd.txt
load 505.mcf_r-32-256K/miss.txt
rdvs(42,:) = rdd(1,:);
hits(42,:) = rdd(2,:);
hitrate(42) = miss(1) / miss(2);
rfunc(42,:) = rdd(2,:) ./ rdd(1,:);
ageHits(42,:) = rdd(4,:);
afunc(42,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-32-512K/rdd.txt
load 505.mcf_r-32-512K/miss.txt
rdvs(43,:) = rdd(1,:);
hits(43,:) = rdd(2,:);
hitrate(43) = miss(1) / miss(2);
rfunc(43,:) = rdd(2,:) ./ rdd(1,:);
ageHits(43,:) = rdd(4,:);
afunc(43,:) = rdd(4,:) ./ rdd(3,:);

load 505.mcf_r-32-1M/rdd.txt
load 505.mcf_r-32-1M/miss.txt
rdvs(44,:) = rdd(1,:);
hits(44,:) = rdd(2,:);
hitrate(44) = miss(1) / miss(2);
rfunc(44,:) = rdd(2,:) ./ rdd(1,:);
ageHits(44,:) = rdd(4,:);
afunc(44,:) = rdd(4,:) ./ rdd(3,:);

load 505.mcf_r-32-2M/rdd.txt
load 505.mcf_r-32-2M/miss.txt
rdvs(45,:) = rdd(1,:);
hits(45,:) = rdd(2,:);
hitrate(45) = miss(1) / miss(2);
rfunc(45,:) = rdd(2,:) ./ rdd(1,:);
ageHits(45,:) = rdd(4,:);
afunc(45,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-64-8K/rdd.txt
load 505.mcf_r-64-8K/miss.txt
rdvs(46,:) = rdd(1,:);
hits(46,:) = rdd(2,:);
hitrate(46) = miss(1) / miss(2);
rfunc(46,:) = rdd(2,:) ./ rdd(1,:);
ageHits(46,:) = rdd(4,:);
afunc(46,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-64-16K/rdd.txt
load 505.mcf_r-64-16K/miss.txt
rdvs(47,:) = rdd(1,:);
hits(47,:) = rdd(2,:);
hitrate(47) = miss(1) / miss(2);
rfunc(47,:) = rdd(2,:) ./ rdd(1,:);
ageHits(47,:) = rdd(4,:);
afunc(47,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-64-32K/rdd.txt
load 505.mcf_r-64-32K/miss.txt
rdvs(48,:) = rdd(1,:);
hits(48,:) = rdd(2,:);
hitrate(48) = miss(1) / miss(2);
rfunc(48,:) = rdd(2,:) ./ rdd(1,:);
ageHits(48,:) = rdd(4,:);
afunc(48,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-64-64K/rdd.txt
load 505.mcf_r-64-64K/miss.txt
rdvs(49,:) = rdd(1,:);
hits(49,:) = rdd(2,:);
hitrate(49) = miss(1) / miss(2);
rfunc(49,:) = rdd(2,:) ./ rdd(1,:);
ageHits(49,:) = rdd(4,:);
afunc(49,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-64-128K/rdd.txt
load 505.mcf_r-64-128K/miss.txt
rdvs(50,:) = rdd(1,:);
hits(50,:) = rdd(2,:);
hitrate(50) = miss(1) / miss(2);
rfunc(50,:) = rdd(2,:) ./ rdd(1,:);
ageHits(50,:) = rdd(4,:);
afunc(50,:) = rdd(4,:) ./ rdd(3,:);

load 505.mcf_r-64-256K/rdd.txt
load 505.mcf_r-64-256K/miss.txt
rdvs(51,:) = rdd(1,:);
hits(51,:) = rdd(2,:);
hitrate(51) = miss(1) / miss(2);
rfunc(51,:) = rdd(2,:) ./ rdd(1,:);
ageHits(51,:) = rdd(4,:);
afunc(51,:) = rdd(4,:) ./ rdd(3,:);

load 505.mcf_r-64-512K/rdd.txt
load 505.mcf_r-64-512K/miss.txt
rdvs(52,:) = rdd(1,:);
hits(52,:) = rdd(2,:);
hitrate(52) = miss(1) / miss(2);
rfunc(52,:) = rdd(2,:) ./ rdd(1,:);
ageHits(52,:) = rdd(4,:);
afunc(52,:) = rdd(4,:) ./ rdd(3,:);

load 505.mcf_r-64-1M/rdd.txt
load 505.mcf_r-64-1M/miss.txt
rdvs(53,:) = rdd(1,:);
hits(53,:) = rdd(2,:);
hitrate(53) = miss(1) / miss(2);
rfunc(53,:) = rdd(2,:) ./ rdd(1,:);
ageHits(53,:) = rdd(4,:);
afunc(53,:) = rdd(4,:) ./ rdd(3,:);
load 505.mcf_r-64-2M/rdd.txt
load 505.mcf_r-64-2M/miss.txt
rdvs(54,:) = rdd(1,:);
hits(54,:) = rdd(2,:);
hitrate(54) = miss(1) / miss(2);
rfunc(54,:) = rdd(2,:) ./ rdd(1,:);
ageHits(54,:) = rdd(4,:);
afunc(54,:) = rdd(4,:) ./ rdd(3,:);
rdvs=rdvs./repmat(sum(abs(rdvs),2),1,size(rdvs,2));
ageHits=ageHits./repmat(sum(abs(ageHits),2),1,size(ageHits,2));
%{
rdd = rdvs(34,:);
fi = zeros(1, size(rdd,2));
for i = 1:size(fi,2)-1
    fi(i) = sum(rdd(i+1:size(rdd,2)));
end
es = zeros(1,512);
for i = 1:size(rdvs,2)
    es(i) = sum(fi(1:i));
    if es(i) < 0
        es(i) = 0;
    end
end
%es(2)=1;

srdd = zeros(size(rdd));

p2 = 0.0;
for i = 1:size(srdd,2)
        if es(i)
            p = (i / es(i)) / (i / es(i) + 1);
        else
            p = 0.5;
        end
        p2 = p2 + p * rdd(i);
end

for i = 1:size(srdd,2)
    sump = 0.0;
    for j = i:size(srdd,2)
        sump = sump + binopdf(i,j,0.5) * rdd(j);
    end
    srdd(i) = sump;
end


for i = 1:size(srdd,2)
    sump = 0.0;
    for j = i:size(srdd,2)
        k = round(i / (j / es(j)));
        sump = sump + binopdf(k,j,0.5) * rdd(j);
    end
    srdd(i) = sump;
end
srdd=srdd./repmat(sum(abs(srdd),2),1,size(srdd,2));
%}
