in_file="Syn.Keep.1v1.1730699.out"
out_file="op_Syn.Keep.1v1.1730699.out"
{
	echo "------T_total_send--------";
	grep 'T_total_send' $in_file |cut -d ',' -f 5 |cut -c 15-20 | sed '0~4 a\\';
	echo "------T_comp_write---------";
	grep 'T_comp_write' $in_file |cut -d ',' -f 1 |cut -c 37-42 | sed '0~4 a\\';
	echo "------T_ana_write---------";
	grep 'T_ana_write' $in_file |cut -d ',' -f 1 |cut -c 35-40 | sed '0~4 a\\';
	echo "------T_create---------";
	grep 'T_create' $in_file |cut -d ',' -f 1 |cut -c 36-41 | sed '0~4 a\\';
	echo "------overlap---------";
	grep 'overlap' $in_file |cut -d ',' -f 4 |cut -c 10-14 | sed '0~4 a\\';
}>> $out_file
