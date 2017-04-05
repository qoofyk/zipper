

grep "Consumer" debug.txt >A_consumer.txt
grep "Writer1" debug.txt  >A_writer.txt
grep "Writer0" debug.txt  >C_writer.txt
grep "Job" debug.txt 	  >C_complete.txt
grep "Receiver3" debug.txt >A_receiver.txt
grep "Reader0" debug.txt >A_reader.txt
grep "EXIT_MSG_TAG" debug.txt > EXIT_MSG_TAG.txt
grep "mpi_recv_progress_counter" debug.txt > mpi_recv_progress_counter.txt

grep "Ana_Proc11" debug.txt >Ana_Proc11.txt
