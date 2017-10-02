log="Syn.Keep.392v196.1801168.out"
in_file="$log"
out_file="op_$log"
folder=MODEL
mkdir -pv $folder

num_comp=392
num_ana=196
repeat=1
group=3

# {
# 	echo "------T_total_send--------";
# 	grep 'T_total_send' $in_file |cut -d ',' -f 5 |cut -c 15-21 | sed '0~4 a\\';
# 	echo "------T_comp_write---------";
# 	grep 'T_comp_write' $in_file |cut -d ',' -f 1 |cut -c 37-43 | sed '0~4 a\\';
# 	echo "------T_ana_write---------";
# 	grep 'T_ana_write' $in_file |cut -d ',' -f 1 |cut -c 35-41 | sed '0~4 a\\';
# 	echo "------T_create---------";
# 	grep 'T_create' $in_file |cut -d ',' -f 1 |cut -c 36-42 | sed '0~4 a\\';
# 	echo "------overlap---------";
# 	grep 'overlap' $in_file |cut -d ',' -f 4 |cut -c 10-15 | sed '0~4 a\\';
# 	echo "------T_ana_total---------";
# 	grep 'T_ana_total' $in_file |cut -d ',' -f 2 |cut -c 14-21 | sed '0~4 a\\';
# }>> $out_file

# GET_LEFT=
# GET_RIGHT="awk 'BEGIN{ORS=""}{print $0 " "; if((NR)%14==0) print "\n" }' > $folder/$name.$out_file"

# GET_MAX="cat $folder/${name}.$out_file | awk '{ maxval=0; for(i=1;i<=NF;i++){if($i>maxval) maxval=$i;} print maxval;}' | awk 'BEGIN{ORS=""}{print $0 " "; if((NR)%1==0) print "\n" }' | sed '0~11 a\\'> $folder/max.${name}.$out_file)"


# Left part
name=Create
grep 'T_create' $in_file |cut -d ',' -f 3 |cut -c 11- | awk -v awk_num_comp="$num_comp" 'BEGIN{ORS=""}{print $0 " "; if((NR)%awk_num_comp==0) print "\n" }' > $folder/$name.$out_file
cat $folder/$name.$out_file | awk '{ maxval=0; for(i=1;i<=NF;i++){if($i>maxval) maxval=$i;} print maxval;}' | awk -v repeat="$repeat" 'BEGIN{ORS=""}{print $0 " "; if((NR)%repeat==0) print "\n" }' | awk -v group="$group" '{print $0; if((NR)%group==0) print ""}'> $folder/max.$name.$out_file
cat $folder/$name.$out_file | awk '{ sum=0; for(i=1;i<=NF;i++){sum+=$i;} print sum/NF;}' | awk -v repeat="$repeat" 'BEGIN{ORS=""}{print $0 " "; if((NR)%repeat==0) print "\n" }' | awk -v group="$group" '{print $0; if((NR)%group==0) print ""}'> $folder/avg.$name.$out_file

name=Comp_Total
grep 'T_create' $in_file |cut -d ',' -f 2 |cut -c 10- | awk -v awk_num_comp="$num_comp" 'BEGIN{ORS=""}{print $0 " "; if((NR)%awk_num_comp==0) print "\n" }' > $folder/$name.$out_file
cat $folder/$name.$out_file | awk '{ maxval=0; for(i=1;i<=NF;i++){if($i>maxval) maxval=$i;} print maxval;}' | awk -v repeat="$repeat" 'BEGIN{ORS=""}{print $0 " "; if((NR)%repeat==0) print "\n" }' | awk -v group="$group" '{print $0; if((NR)%group==0) print ""}'> $folder/max.$name.$out_file

name=Send
grep 'T_total_send' $in_file |cut -d ',' -f 5 |cut -c 15- |awk -v awk_num_comp="$num_comp" 'BEGIN{ORS=""}{print $0 " "; if((NR)%awk_num_comp==0) print "\n" }' > $folder/$name.$out_file
cat $folder/$name.$out_file | awk '{ maxval=0; for(i=1;i<=NF;i++){if($i>maxval) maxval=$i;} print maxval;}' | awk -v repeat="$repeat" 'BEGIN{ORS=""}{print $0 " "; if((NR)%repeat==0) print "\n" }' | awk -v group="$group" '{print $0; if((NR)%group==0) print ""}'> $folder/max.$name.$out_file

name=Comp_write
grep 'T_comp_write' $in_file |cut -d ',' -f 2 |cut -c 15- | awk -v awk_num_comp="$num_comp" 'BEGIN{ORS=""}{print $0 " "; if((NR)%awk_num_comp==0) print "\n" }' > $folder/$name.$out_file
cat $folder/$name.$out_file | awk '{ maxval=0; for(i=1;i<=NF;i++){if($i>maxval) maxval=$i;} print maxval;}' | awk -v repeat="$repeat" 'BEGIN{ORS=""}{print $0 " "; if((NR)%repeat==0) print "\n" }' | awk -v group="$group" '{print $0; if((NR)%group==0) print ""}'> $folder/max.$name.$out_file

name=Overlap
grep 'overlap' $in_file |cut -d ',' -f 5 |cut -c 10- | awk -v awk_num_comp="$num_comp" 'BEGIN{ORS=""}{print $0 " "; if((NR)%awk_num_comp==0) print "\n" }' > $folder/$name.$out_file
cat $folder/$name.$out_file | awk '{ maxval=0; for(i=1;i<=NF;i++){if($i>maxval) maxval=$i;} print maxval;}' | awk -v repeat="$repeat" 'BEGIN{ORS=""}{print $0 " "; if((NR)%repeat==0) print "\n" }' | awk -v group="$group" '{print $0; if((NR)%group==0) print ""}'> $folder/max.$name.$out_file


# Right part
name=Ana_write
grep 'T_ana_write' $in_file |cut -d ',' -f 3 |cut -c 14- | awk -v num_ana="$num_ana" 'BEGIN{ORS=""}{print $0 " "; if((NR)%num_ana==0) print "\n" }' > $folder/$name.$out_file
cat $folder/$name.$out_file | awk '{ maxval=0; for(i=1;i<=NF;i++){if($i>maxval) maxval=$i;} print maxval;}' | awk -v repeat="$repeat" 'BEGIN{ORS=""}{print $0 " "; if((NR)%repeat==0) print "\n" }' | awk -v group="$group" '{print $0; if((NR)%group==0) print ""}'> $folder/max.$name.$out_file

name=Ana_read
grep 'T_ana_read' $in_file |cut -d ',' -f 2 |cut -c 13- | awk -v num_ana="$num_ana" 'BEGIN{ORS=""}{print $0 " "; if((NR)%num_ana==0) print "\n" }' > $folder/$name.$out_file
cat $folder/$name.$out_file | awk '{ maxval=0; for(i=1;i<=NF;i++){if($i>maxval) maxval=$i;} print maxval;}' | awk -v repeat="$repeat" 'BEGIN{ORS=""}{print $0 " "; if((NR)%repeat==0) print "\n" }' | awk -v group="$group" '{print $0; if((NR)%group==0) print ""}'> $folder/max.$name.$out_file

name=Calc
grep 'T_calc' $in_file |cut -d ',' -f 2 |cut -c 9- | awk -v num_ana="$num_ana" 'BEGIN{ORS=""}{print $0 " "; if((NR)%num_ana==0) print "\n" }' > $folder/$name.$out_file
cat $folder/$name.$out_file | awk '{ maxval=0; for(i=1;i<=NF;i++){if($i>maxval) maxval=$i;} print maxval;}' | awk -v repeat="$repeat" 'BEGIN{ORS=""}{print $0 " "; if((NR)%repeat==0) print "\n" }' | awk -v group="$group" '{print $0; if((NR)%group==0) print ""}'> $folder/max.$name.$out_file
cat $folder/$name.$out_file | awk '{ sum=0; for(i=1;i<=NF;i++){sum+=$i;} print sum/NF;}' | awk -v repeat="$repeat" 'BEGIN{ORS=""}{print $0 " "; if((NR)%repeat==0) print "\n" }' | awk -v group="$group" '{print $0; if((NR)%group==0) print ""}'> $folder/avg.$name.$out_file

name=Total
															# change column to line
grep 'T_ana_total' $in_file |cut -d ',' -f 2 |cut -c 14- | awk -v num_ana="$num_ana" 'BEGIN{ORS=""}{print $0 " "; if((NR)%num_ana==0) print "\n" }' > $folder/$name.$out_file
							# get max of this line															# change column to line	for every repeat times						#add space
cat $folder/$name.$out_file | awk '{ maxval=0; for(i=1;i<=NF;i++){if($i>maxval) maxval=$i;} print maxval;}' | awk -v repeat="$repeat" 'BEGIN{ORS=""}{print $0 " "; if((NR)%repeat==0) print "\n" }' | awk -v group="$group" '{print $0; if((NR)%group==0) print ""}'> $folder/max.$name.$out_file

name=Wrong
grep 'Consumer2' $in_file |cut -d ',' -f 9 |cut -c 8- | awk -v num_ana="$num_ana" 'BEGIN{ORS=""}{print $0 " "; if((NR)%num_ana==0) print "\n" }' > $folder/$name.$out_file
cat $folder/$name.$out_file | awk '{ maxval=0; for(i=1;i<=NF;i++){if($i>maxval) maxval=$i;} print maxval;}' | awk -v repeat="$repeat" 'BEGIN{ORS=""}{print $0 " "; if((NR)%repeat==0) print "\n" }' | awk -v group="$group" '{print $0; if((NR)%group==0) print ""}'> $folder/max.$name.$out_file

name=Read_wrong
grep 'read_wrong' $in_file |cut -d ',' -f 9 |cut -c 13- | awk -v num_ana="$num_ana" 'BEGIN{ORS=""}{print $0 " "; if((NR)%num_ana==0) print "\n" }' > $folder/$name.$out_file
cat $folder/$name.$out_file | awk '{ maxval=0; for(i=1;i<=NF;i++){if($i>maxval) maxval=$i;} print maxval;}' | awk -v repeat="$repeat" 'BEGIN{ORS=""}{print $0 " "; if((NR)%repeat==0) print "\n" }' | awk -v group="$group" '{print $0; if((NR)%group==0) print ""}'> $folder/max.$name.$out_file

