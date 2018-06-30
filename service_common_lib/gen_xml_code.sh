#!/usr/bin/env bash

#set tool path
xml_tool="../../../../tools/xml_to_c++/xmlToC++"
chmod +x ${xml_tool}


#set param
output_src_dir="."
xml_file_dir="../../../../run/service/common/config"
xml_config_file="${xml_file_dir}/common.xml"

#gen code
${xml_tool} ${xml_config_file} ${output_src_dir}


#set param
output_src_dir="."
xml_file_dir="../../../../run/service/common/config"
xml_config_file="${xml_file_dir}/mall.xml"

#gen code
${xml_tool} ${xml_config_file} ${output_src_dir}


#set param
output_src_dir="."
xml_file_dir="../../../../run/service/common/config"
xml_config_file="${xml_file_dir}/db.xml"

#gen code
${xml_tool} ${xml_config_file} ${output_src_dir}


#set param
output_src_dir="."
xml_file_dir="../../../../run/service/common/config"
xml_config_file="${xml_file_dir}/service.xml"

#gen code
${xml_tool} ${xml_config_file} ${output_src_dir}


#set param
output_src_dir="."
xml_file_dir="../../../../run/service/common/config"
xml_config_file="${xml_file_dir}/cattles.xml"

#gen code
${xml_tool} ${xml_config_file} ${output_src_dir}


#set param
output_src_dir="."
xml_file_dir="../../../../run/service/common/config"
xml_config_file="${xml_file_dir}/golden_fraud.xml"

#gen code
${xml_tool} ${xml_config_file} ${output_src_dir}


#set param
output_src_dir="."
xml_file_dir="../../../../run/service/common/config"
xml_config_file="${xml_file_dir}/fight_landlord.xml"

#gen code
${xml_tool} ${xml_config_file} ${output_src_dir}
