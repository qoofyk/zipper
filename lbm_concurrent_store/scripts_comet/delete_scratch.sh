output_dir=/oasis/scratch/comet/qoofyk/temp_project/LBMconcurrentstore/
empty_dir=/oasis/scratch/comet/qoofyk/temp_project/empty/

rm -rf ${empty_dir}
mkdir ${empty_dir}

time rsync -a --delete-before ${empty_dir} ${output_dir}
# time rsync -a --delete-before /oasis/scratch/comet/qoofyk/temp_project/empty /oasis/scratch/comet/qoofyk/temp_project/LBMconcurrentstore

# rm -rf ${output_dir}

# ls -lh ${output_dir}
