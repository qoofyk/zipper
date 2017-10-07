make -f Makefile.nokeep.onefile clean
make -j28 -f Makefile.nokeep.onefile 1>make.log 2>make.err
#make -j28 -f Makefile.nokeep.bridges.onefile &>>make.log
