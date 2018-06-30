#!/usr/bin/env bash

#set tool path
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:../../../../import/third_party/lib
proto_tool="../../../../import/third_party/bin/protoc"
chmod +x ${proto_tool}
 
#set param
output_src_dir="./protocol"
proto_file_dir="../../../../src/common/protocol"
proto_project_file="${proto_file_dir}/golden_fraud_game.proto"
proto_file_lst=${proto_project_file}

#copy service proto file to total dir
service_file_dir="${proto_file_dir}/golden_fraud_game"
cp ${service_file_dir}/* ${proto_file_dir}/

#get all import protocol file
input_file_lst=${proto_file_lst}

while [[ -n ${input_file_lst} ]]; do
cur_file_lst=$(awk -F '"' -v proto_dir=${proto_file_dir} '{
	if(NF == 3)
	{
		if(substr($1,1,6) == "import")
		{
			if (FILENAME == "'${proto_project_file}'")
			{
				printf("%s/%s ", proto_dir, $2);
			}
			else
			{
			    printf("%s/%s ", proto_dir, $2);
			}
		}
	}
}' ${input_file_lst})
input_file_lst=${cur_file_lst}
proto_file_lst="${proto_file_lst} ${cur_file_lst}"
done;

proto_file_lst=$(echo ${proto_file_lst} | awk -F ' ' '{
	for( i = 1; i <= NF; i++)
	{
		filepath[$i]++;
	}
}
END{
	for( i in filepath)
	{
		printf("%s ", i);
	}
}')

echo "proto_file_lst:${proto_file_lst}"

#gen code
${proto_tool} --proto_path=${proto_file_dir} --cpp_out=${output_src_dir} ${proto_file_lst}
