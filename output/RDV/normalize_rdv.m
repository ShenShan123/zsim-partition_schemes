rdvs = zeros(30,512);
hits = zeros(30,512);
func = zeros(30,512);
hitrate = zeros(30,1);

load 500.perlbench_r-16-8K/rdd.txt
load 500.perlbench_r-16-8K/miss.txt
rdvs(1,:) = rdd(1,:);
hits(1,:) = rdd(2,:);
ages(1,:) = rdd(3,:);
hitrate(1) = miss(1) / miss(2);
func(1,:) = rdd(3,:) ./ rdd(2,:);
load 500.perlbench_r-16-16K/rdd.txt
load 500.perlbench_r-16-16K/miss.txt
rdvs(2,:) = rdd(1,:);
hits(2,:) = rdd(2,:);
hitrate(2) = miss(1) / miss(2);
func(2,:) = rdd(3,:) ./ rdd(2,:);
load 500.perlbench_r-16-32K/rdd.txt
load 500.perlbench_r-16-32K/miss.txt
rdvs(3,:) = rdd(1,:);
hits(3,:) = rdd(2,:);
hitrate(3) = miss(1) / miss(2);
func(3,:) = rdd(3,:) ./ rdd(2,:);
load 500.perlbench_r-16-64K/rdd.txt
load 500.perlbench_r-16-64K/miss.txt
rdvs(4,:) = rdd(1,:);
hits(4,:) = rdd(2,:);
hitrate(4) = miss(1) / miss(2);
func(4,:) = rdd(3,:) ./ rdd(2,:);
load 500.perlbench_r-16-128K/rdd.txt
load 500.perlbench_r-16-128K/miss.txt
rdvs(5,:) = rdd(1,:);
hits(5,:) = rdd(2,:);
hitrate(5) = miss(1) / miss(2);
func(5,:) = rdd(3,:) ./ rdd(2,:);
load 500.perlbench_r-16-256K/rdd.txt
load 500.perlbench_r-16-256K/miss.txt
rdvs(6,:) = rdd(1,:);
hits(6,:) = rdd(2,:);
hitrate(6) = miss(1) / miss(2);
func(6,:) = rdd(3,:) ./ rdd(2,:);
load 500.perlbench_r-16-512K/rdd.txt
load 500.perlbench_r-16-512K/miss.txt
rdvs(7,:) = rdd(1,:);
hits(7,:) = rdd(2,:);
hitrate(7) = miss(1) / miss(2);
func(7,:) = rdd(3,:) ./ rdd(2,:);
load 500.perlbench_r-16-1M/rdd.txt
load 500.perlbench_r-16-1M/miss.txt
rdvs(8,:) = rdd(1,:);
hits(8,:) = rdd(2,:);
hitrate(8) = miss(1) / miss(2);
func(8,:) = rdd(3,:) ./ rdd(2,:);
load 500.perlbench_r-16-2M/rdd.txt
load 500.perlbench_r-16-2M/miss.txt
rdvs(9,:) = rdd(1,:);
hits(9,:) = rdd(2,:);
hitrate(9) = miss(1) / miss(2);
func(9,:) = rdd(3,:) ./ rdd(2,:);

load 505.mcf_r-16-8K/rdd.txt
load 505.mcf_r-16-8K/miss.txt
rdvs(1,:) = rdd(1,:);
hits(1,:) = rdd(2,:);
ages(1,:) = rdd(3,:);
hitrate(1) = miss(1) / miss(2);
func(1,:) = rdd(3,:) ./ rdd(2,:);
load 505.mcf_r-16-16K/rdd.txt
load 505.mcf_r-16-16K/miss.txt
rdvs(2,:) = rdd(1,:);
hits(2,:) = rdd(2,:);
hitrate(2) = miss(1) / miss(2);
func(2,:) = rdd(3,:) ./ rdd(2,:);
load 505.mcf_r-16-32K/rdd.txt
load 505.mcf_r-16-32K/miss.txt
rdvs(3,:) = rdd(1,:);
hits(3,:) = rdd(2,:);
hitrate(3) = miss(1) / miss(2);
func(3,:) = rdd(3,:) ./ rdd(2,:);
load 505.mcf_r-16-64K/rdd.txt
load 505.mcf_r-16-64K/miss.txt
rdvs(4,:) = rdd(1,:);
hits(4,:) = rdd(2,:);
hitrate(4) = miss(1) / miss(2);
func(4,:) = rdd(3,:) ./ rdd(2,:);
load 505.mcf_r-16-128K/rdd.txt
load 505.mcf_r-16-128K/miss.txt
rdvs(5,:) = rdd(1,:);
hits(5,:) = rdd(2,:);
hitrate(5) = miss(1) / miss(2);
func(5,:) = rdd(3,:) ./ rdd(2,:);
load 505.mcf_r-16-256K/rdd.txt
load 505.mcf_r-16-256K/miss.txt
rdvs(6,:) = rdd(1,:);
hits(6,:) = rdd(2,:);
hitrate(6) = miss(1) / miss(2);
func(6,:) = rdd(3,:) ./ rdd(2,:);
load 505.mcf_r-16-512K/rdd.txt
load 505.mcf_r-16-512K/miss.txt
rdvs(7,:) = rdd(1,:);
hits(7,:) = rdd(2,:);
hitrate(7) = miss(1) / miss(2);
func(7,:) = rdd(3,:) ./ rdd(2,:);
load 505.mcf_r-16-1M/rdd.txt
load 505.mcf_r-16-1M/miss.txt
rdvs(8,:) = rdd(1,:);
hits(8,:) = rdd(2,:);
hitrate(8) = miss(1) / miss(2);
func(8,:) = rdd(3,:) ./ rdd(2,:);
load 505.mcf_r-16-2M/rdd.txt
load 505.mcf_r-16-2M/miss.txt
rdvs(9,:) = rdd(1,:);
hits(9,:) = rdd(2,:);
hitrate(9) = miss(1) / miss(2);
func(9,:) = rdd(3,:) ./ rdd(2,:);

load 507.cactuBSSN_r-16-8K/rdd.txt
load 507.cactuBSSN_r-16-8K/miss.txt
rdvs(1,:) = rdd(1,:);
hits(1,:) = rdd(2,:);
ages(1,:) = rdd(3,:);
hitrate(1) = miss(1) / miss(2);
func(1,:) = rdd(3,:) ./ rdd(2,:);
load 507.cactuBSSN_r-16-16K/rdd.txt
load 507.cactuBSSN_r-16-16K/miss.txt
rdvs(2,:) = rdd(1,:);
hits(2,:) = rdd(2,:);
hitrate(2) = miss(1) / miss(2);
func(2,:) = rdd(3,:) ./ rdd(2,:);
load 507.cactuBSSN_r-16-32K/rdd.txt
load 507.cactuBSSN_r-16-32K/miss.txt
rdvs(3,:) = rdd(1,:);
hits(3,:) = rdd(2,:);
hitrate(3) = miss(1) / miss(2);
func(3,:) = rdd(3,:) ./ rdd(2,:);
load 507.cactuBSSN_r-16-64K/rdd.txt
load 507.cactuBSSN_r-16-64K/miss.txt
rdvs(4,:) = rdd(1,:);
hits(4,:) = rdd(2,:);
hitrate(4) = miss(1) / miss(2);
func(4,:) = rdd(3,:) ./ rdd(2,:);
load 507.cactuBSSN_r-16-128K/rdd.txt
load 507.cactuBSSN_r-16-128K/miss.txt
rdvs(5,:) = rdd(1,:);
hits(5,:) = rdd(2,:);
hitrate(5) = miss(1) / miss(2);
func(5,:) = rdd(3,:) ./ rdd(2,:);
load 507.cactuBSSN_r-16-256K/rdd.txt
load 507.cactuBSSN_r-16-256K/miss.txt
rdvs(6,:) = rdd(1,:);
hits(6,:) = rdd(2,:);
hitrate(6) = miss(1) / miss(2);
func(6,:) = rdd(3,:) ./ rdd(2,:);
load 507.cactuBSSN_r-16-512K/rdd.txt
load 507.cactuBSSN_r-16-512K/miss.txt
rdvs(7,:) = rdd(1,:);
hits(7,:) = rdd(2,:);
hitrate(7) = miss(1) / miss(2);
func(7,:) = rdd(3,:) ./ rdd(2,:);
load 507.cactuBSSN_r-16-1M/rdd.txt
load 507.cactuBSSN_r-16-1M/miss.txt
rdvs(8,:) = rdd(1,:);
hits(8,:) = rdd(2,:);
hitrate(8) = miss(1) / miss(2);
func(8,:) = rdd(3,:) ./ rdd(2,:);
load 507.cactuBSSN_r-16-2M/rdd.txt
load 507.cactuBSSN_r-16-2M/miss.txt
rdvs(9,:) = rdd(1,:);
hits(9,:) = rdd(2,:);
hitrate(9) = miss(1) / miss(2);
func(9,:) = rdd(3,:) ./ rdd(2,:);

load  531.deepsjeng_r-16-8K/rdd.txt
load  531.deepsjeng_r-16-8K/miss.txt
rdvs(1,:) = rdd(1,:);
hits(1,:) = rdd(2,:);
ages(1,:) = rdd(3,:);
hitrate(1) = miss(1) / miss(2);
func(1,:) = rdd(3,:) ./ rdd(2,:);
load  531.deepsjeng_r-16-16K/rdd.txt
load  531.deepsjeng_r-16-16K/miss.txt
rdvs(2,:) = rdd(1,:);
hits(2,:) = rdd(2,:);
hitrate(2) = miss(1) / miss(2);
func(2,:) = rdd(3,:) ./ rdd(2,:);
load  531.deepsjeng_r-16-32K/rdd.txt
load  531.deepsjeng_r-16-32K/miss.txt
rdvs(3,:) = rdd(1,:);
hits(3,:) = rdd(2,:);
hitrate(3) = miss(1) / miss(2);
func(3,:) = rdd(3,:) ./ rdd(2,:);
load  531.deepsjeng_r-16-64K/rdd.txt
load  531.deepsjeng_r-16-64K/miss.txt
rdvs(4,:) = rdd(1,:);
hits(4,:) = rdd(2,:);
hitrate(4) = miss(1) / miss(2);
func(4,:) = rdd(3,:) ./ rdd(2,:);
load  531.deepsjeng_r-16-128K/rdd.txt
load  531.deepsjeng_r-16-128K/miss.txt
rdvs(5,:) = rdd(1,:);
hits(5,:) = rdd(2,:);
hitrate(5) = miss(1) / miss(2);
func(5,:) = rdd(3,:) ./ rdd(2,:);
load  531.deepsjeng_r-16-256K/rdd.txt
load  531.deepsjeng_r-16-256K/miss.txt
rdvs(6,:) = rdd(1,:);
hits(6,:) = rdd(2,:);
hitrate(6) = miss(1) / miss(2);
func(6,:) = rdd(3,:) ./ rdd(2,:);
load  531.deepsjeng_r-16-512K/rdd.txt
load  531.deepsjeng_r-16-512K/miss.txt
rdvs(7,:) = rdd(1,:);
hits(7,:) = rdd(2,:);
hitrate(7) = miss(1) / miss(2);
func(7,:) = rdd(3,:) ./ rdd(2,:);
load  531.deepsjeng_r-16-1M/rdd.txt
load  531.deepsjeng_r-16-1M/miss.txt
rdvs(8,:) = rdd(1,:);
hits(8,:) = rdd(2,:);
hitrate(8) = miss(1) / miss(2);
func(8,:) = rdd(3,:) ./ rdd(2,:);
load  531.deepsjeng_r-16-2M/rdd.txt
load  531.deepsjeng_r-16-2M/miss.txt
rdvs(9,:) = rdd(1,:);
hits(9,:) = rdd(2,:);
hitrate(9) = miss(1) / miss(2);
func(9,:) = rdd(3,:) ./ rdd(2,:);

load 538.imagick_r-16-8K/rdd.txt
load 538.imagick_r-16-8K/miss.txt
rdvs(1,:) = rdd(1,:);
hits(1,:) = rdd(2,:);
ages(1,:) = rdd(3,:);
hitrate(1) = miss(1) / miss(2);
func(1,:) = rdd(3,:) ./ rdd(2,:);
load 538.imagick_r-16-16K/rdd.txt
load 538.imagick_r-16-16K/miss.txt
rdvs(2,:) = rdd(1,:);
hits(2,:) = rdd(2,:);
hitrate(2) = miss(1) / miss(2);
func(2,:) = rdd(3,:) ./ rdd(2,:);
load 538.imagick_r-16-32K/rdd.txt
load 538.imagick_r-16-32K/miss.txt
rdvs(3,:) = rdd(1,:);
hits(3,:) = rdd(2,:);
hitrate(3) = miss(1) / miss(2);
func(3,:) = rdd(3,:) ./ rdd(2,:);
load 538.imagick_r-16-64K/rdd.txt
load 538.imagick_r-16-64K/miss.txt
rdvs(4,:) = rdd(1,:);
hits(4,:) = rdd(2,:);
hitrate(4) = miss(1) / miss(2);
func(4,:) = rdd(3,:) ./ rdd(2,:);
load 538.imagick_r-16-128K/rdd.txt
load 538.imagick_r-16-128K/miss.txt
rdvs(5,:) = rdd(1,:);
hits(5,:) = rdd(2,:);
hitrate(5) = miss(1) / miss(2);
func(5,:) = rdd(3,:) ./ rdd(2,:);
load 538.imagick_r-16-256K/rdd.txt
load 538.imagick_r-16-256K/miss.txt
rdvs(6,:) = rdd(1,:);
hits(6,:) = rdd(2,:);
hitrate(6) = miss(1) / miss(2);
func(6,:) = rdd(3,:) ./ rdd(2,:);
load 538.imagick_r-16-512K/rdd.txt
load 538.imagick_r-16-512K/miss.txt
rdvs(7,:) = rdd(1,:);
hits(7,:) = rdd(2,:);
hitrate(7) = miss(1) / miss(2);
func(7,:) = rdd(3,:) ./ rdd(2,:);
load 538.imagick_r-16-1M/rdd.txt
load 538.imagick_r-16-1M/miss.txt
rdvs(8,:) = rdd(1,:);
hits(8,:) = rdd(2,:);
hitrate(8) = miss(1) / miss(2);
func(8,:) = rdd(3,:) ./ rdd(2,:);
load 538.imagick_r-16-2M/rdd.txt
load 538.imagick_r-16-2M/miss.txt
rdvs(9,:) = rdd(1,:);
hits(9,:) = rdd(2,:);
hitrate(9) = miss(1) / miss(2);
func(9,:) = rdd(3,:) ./ rdd(2,:);

%{
load 500.perlbench_r-8-32K/rdd.txt
rdvs(1,:) = rdd(1,:);
hits(1,:) = rdd(2,:);
func(1,:) = rdd(3,:) ./ rdd(2,:);
load 500.perlbench_r-16-512K/rdd.txt
rdvs(2,:) = rdd(1,:);
hits(2,:) = rdd(2,:);
func(2,:) = rdd(3,:) ./ rdd(2,:);
load 500.perlbench_r-32-1M/rdd.txt
rdvs(3,:) = rdd(1,:);
hits(3,:) = rdd(2,:);
func(3,:) = rdd(3,:) ./ rdd(2,:);
load 500.perlbench_r-64-2M/rdd.txt
rdvs(4,:) = rdd(1,:);
hits(4,:) = rdd(2,:);
func(4,:) = rdd(3,:) ./ rdd(2,:);
%{
load 500.perlbench_r-16-1M/rdd.txt
rdvs(5,:) = rdd(1,:);
hits(5,:) = rdd(2,:);
func(5,:) = rdd(3,:) ./ rdd(2,:);
load 500.perlbench_r-16-2M/rdd.txt
rdvs(6,:) = rdd(1,:);
hits(6,:) = rdd(2,:);
func(6,:) = rdd(3,:) ./ rdd(2,:);
%}
load 505.mcf_r-8-32K/rdd.txt
rdvs(7,:) = rdd(1,:);
hits(7,:) = rdd(2,:);
func(7,:) = rdd(3,:) ./ rdd(2,:);
load 505.mcf_r-16-512K/rdd.txt
rdvs(8,:) = rdd(1,:);
hits(8,:) = rdd(2,:);
func(8,:) = rdd(3,:) ./ rdd(2,:);
load 505.mcf_r-32-1M/rdd.txt
rdvs(9,:) = rdd(1,:);
hits(9,:) = rdd(2,:);
func(9,:) = rdd(3,:) ./ rdd(2,:);
load 505.mcf_r-64-2M/rdd.txt
rdvs(10,:) = rdd(1,:);
hits(10,:) = rdd(2,:);
func(10,:) = rdd(3,:) ./ rdd(2,:);
%{
load 505.mcf_r-16-1M/rdd.txt
rdvs(11,:) = rdd(1,:);
hits(11,:) = rdd(2,:);
func(11,:) = rdd(3,:) ./ rdd(2,:);
load 505.mcf_r-16-2M/rdd.txt
rdvs(12,:) = rdd(1,:);
hits(12,:) = rdd(2,:);
func(12,:) = rdd(3,:) ./ rdd(2,:);
%}
load 507.cactuBSSN_r-8-32K/rdd.txt
rdvs(13,:) = rdd(1,:);
hits(13,:) = rdd(2,:);
func(13,:) = rdd(3,:) ./ rdd(2,:);
load 507.cactuBSSN_r-16-512K/rdd.txt
rdvs(14,:) = rdd(1,:);
hits(14,:) = rdd(2,:);
func(14,:) = rdd(3,:) ./ rdd(2,:);
load 507.cactuBSSN_r-32-1M/rdd.txt
rdvs(15,:) = rdd(1,:);
hits(15,:) = rdd(2,:);
func(15,:) = rdd(3,:) ./ rdd(2,:);
load 507.cactuBSSN_r-64-2M/rdd.txt
rdvs(16,:) = rdd(1,:);
hits(16,:) = rdd(2,:);
func(16,:) = rdd(3,:) ./ rdd(2,:);
%{
load 507.cactuBSSN_r-16-1M/rdd.txt
rdvs(17,:) = rdd(1,:);
hits(17,:) = rdd(2,:);
func(17,:) = rdd(3,:) ./ rdd(2,:);
load 507.cactuBSSN_r-16-2M/rdd.txt
rdvs(18,:) = rdd(1,:);
hits(18,:) = rdd(2,:);
func(18,:) = rdd(3,:) ./ rdd(2,:);
%}
load 531.deepsjeng_r-8-32K/rdd.txt
rdvs(19,:) = rdd(1,:);
hits(19,:) = rdd(2,:);
func(19,:) = rdd(3,:) ./ rdd(2,:);
load 531.deepsjeng_r-16-512K/rdd.txt
rdvs(20,:) = rdd(1,:);
hits(20,:) = rdd(2,:);
func(20,:) = rdd(3,:) ./ rdd(2,:);
load 531.deepsjeng_r-32-1M/rdd.txt
rdvs(21,:) = rdd(1,:);
hits(21,:) = rdd(2,:);
func(21,:) = rdd(3,:) ./ rdd(2,:);
load 531.deepsjeng_r-64-2M/rdd.txt
rdvs(22,:) = rdd(1,:);
hits(22,:) = rdd(2,:);
func(22,:) = rdd(3,:) ./ rdd(2,:);
%{
load 531.deepsjeng_r-16-1M/rdd.txt
rdvs(23,:) = rdd(1,:);
hits(23,:) = rdd(2,:);
func(23,:) = rdd(3,:) ./ rdd(2,:);
load 531.deepsjeng_r-16-2M/rdd.txt
rdvs(24,:) = rdd(1,:);
hits(24,:) = rdd(2,:);
func(24,:) = rdd(3,:) ./ rdd(2,:);
%}
load 538.imagick_r-8-32K/rdd.txt
rdvs(25,:) = rdd(1,:);
hits(25,:) = rdd(2,:);
func(25,:) = rdd(3,:) ./ rdd(2,:);
load 538.imagick_r-16-512K/rdd.txt
rdvs(26,:) = rdd(1,:);
hits(26,:) = rdd(2,:);
func(26,:) = rdd(3,:) ./ rdd(2,:);
load 538.imagick_r-32-1M/rdd.txt
rdvs(27,:) = rdd(1,:);
hits(27,:) = rdd(2,:);
func(27,:) = rdd(3,:) ./ rdd(2,:);
load 538.imagick_r-64-2M/rdd.txt
rdvs(28,:) = rdd(1,:);
hits(28,:) = rdd(2,:);
func(28,:) = rdd(3,:) ./ rdd(2,:);
%{
load 538.imagick_r-16-1M/rdd.txt
rdvs(29,:) = rdd(1,:);
hits(29,:) = rdd(2,:);
func(29,:) = rdd(3,:) ./ rdd(2,:);
load 538.imagick_r-16-2M/rdd.txt
rdvs(30,:) = rdd(1,:);
hits(30,:) = rdd(2,:);
func(30,:) = rdd(3,:) ./ rdd(2,:);
%}
%}
rdvs=rdvs./repmat(sum(abs(rdvs),2),1,size(rdvs,2));