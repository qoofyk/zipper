in_file="Syn.Keep.1v1.1702298.out"
out_file="op_Syn.Keep.1v1.1702298.out"
{
	echo "------T_total_send--------";
	grep 'T_total_send' $in_file |cut -d ',' -f 5 |cut -c 15-19 | sed '0~4 a\\';
	echo "------T_comp_write---------";
	grep 'T_comp_write' $in_file |cut -d ',' -f 1 |cut -c 37-41 | sed '0~4 a\\';
	echo "------T_ana_write---------";
	grep 'T_ana_write' $in_file |cut -d ',' -f 1 |cut -c 35-40 | sed '0~4 a\\';
}>> $out_file
