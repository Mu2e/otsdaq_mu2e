puts "JTAG index N: [lindex $argv 0]"
puts "JTAG-N programming mcs file: [lindex $argv 1]"
puts "JTAG-N target flash part: [lindex $argv 2]"

open_hw_manager
connect_hw_server


open_hw_target [lindex [get_hw_targets *] [lindex $argv 0]]

#create_hw_cfgmem -hw_device [lindex [get_hw_devices xc7k325t_0] 0] [lindex [get_cfgmem_parts {mt28gu512aax1e-bpi-x16}] 0]
#create_hw_cfgmem -hw_device [lindex [get_hw_devices] 0] [lindex [get_cfgmem_parts {mt28gu01gaax1e-bpi-x16}] 0]
create_hw_cfgmem -hw_device [lindex [get_hw_devices] 0] [lindex [get_cfgmem_parts [lindex $argv 2]] 0]
set_property PROGRAM.BLANK_CHECK  0 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
set_property PROGRAM.ERASE  1 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
set_property PROGRAM.CFG_PROGRAM  1 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
set_property PROGRAM.VERIFY  1 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
set_property PROGRAM.CHECKSUM  0 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
refresh_hw_device [lindex [get_hw_devices] 0]

set_property PROGRAM.ADDRESS_RANGE  {use_file} [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
set_property PROGRAM.FILES [lindex $argv 1]  [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
#set_property PROGRAM.FILES [list "/home/kwar/dtc/DTC2022Nov22_12_19.2/DTC2022Nov22.mcs" ] [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
set_property PROGRAM.PRM_FILE {} [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
set_property PROGRAM.BPI_RS_PINS {none} [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
set_property PROGRAM.UNUSED_PIN_TERMINATION {pull-none} [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
set_property PROGRAM.BLANK_CHECK  0 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
set_property PROGRAM.ERASE  1 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
set_property PROGRAM.CFG_PROGRAM  1 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
set_property PROGRAM.VERIFY  1 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
set_property PROGRAM.CHECKSUM  0 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
startgroup 
create_hw_bitstream -hw_device [lindex [get_hw_devices ] 0] [get_property PROGRAM.HW_CFGMEM_BITFILE [ lindex [get_hw_devices ] 0]]; program_hw_devices [lindex [get_hw_devices ] 0]; refresh_hw_device [lindex [get_hw_devices ] 0];
program_hw_cfgmem -hw_cfgmem [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
endgroup

boot_hw_device  [lindex [get_hw_devices] 0]
refresh_hw_device [lindex [get_hw_devices] 0]

close_hw_target



disconnect_hw_server
