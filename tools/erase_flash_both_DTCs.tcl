puts "JTAG-0 programming mcs file: [lindex $argv 0]"
puts "JTAG-0 target flash part: [lindex $argv 1]"
puts "JTAG-1 programming mcs file: [lindex $argv 2]"
puts "JTAG-1 target flash part: [lindex $argv 3]"
#puts "Tcl programming mcs file in flash memory mt28gu01gaax1e-bpi-x16: [lindex $argv 0]"
open_hw_manager
connect_hw_server


open_hw_target [lindex [get_hw_targets *] 0]
#create_hw_cfgmem -hw_device [lindex [get_hw_devices xc7k325t_0] 0] [lindex [get_cfgmem_parts {mt28gu512aax1e-bpi-x16}] 0]
#create_hw_cfgmem -hw_device [lindex [get_hw_devices] 0] [lindex [get_cfgmem_parts {mt28gu01gaax1e-bpi-x16}] 0]
create_hw_cfgmem -hw_device [lindex [get_hw_devices] 0] [lindex [get_cfgmem_parts [lindex $argv 1]] 0]
set_property PROGRAM.BLANK_CHECK  0 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
set_property PROGRAM.ERASE  1 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
set_property PROGRAM.CFG_PROGRAM  0 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
set_property PROGRAM.VERIFY  0 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
set_property PROGRAM.CHECKSUM  0 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
set_property PROGRAM.ADDRESS_RANGE  {entire_device} [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
#set_property PROGRAM.BPI_RS_PINS {none} [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
#refresh_hw_device [lindex [get_hw_devices] 0]
startgroup 
create_hw_bitstream -hw_device [lindex [get_hw_devices ] 0] [get_property PROGRAM.HW_CFGMEM_BITFILE [ lindex [get_hw_devices ] 0]]; program_hw_devices [lindex [get_hw_devices ] 0]; refresh_hw_device [lindex [get_hw_devices ] 0];
program_hw_cfgmem -hw_cfgmem [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
endgroup

#boot_hw_device  [lindex [get_hw_devices] 0]
refresh_hw_device [lindex [get_hw_devices] 0]

close_hw_target



open_hw_target [lindex [get_hw_targets *] 1]
#create_hw_cfgmem -hw_device [lindex [get_hw_devices xc7k325t_0] 0] [lindex [get_cfgmem_parts {mt28gu512aax1e-bpi-x16}] 0]
#create_hw_cfgmem -hw_device [lindex [get_hw_devices] 0] [lindex [get_cfgmem_parts {mt28gu01gaax1e-bpi-x16}] 0]
create_hw_cfgmem -hw_device [lindex [get_hw_devices] 0] [lindex [get_cfgmem_parts [lindex $argv 1]] 0]
set_property PROGRAM.BLANK_CHECK  0 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
set_property PROGRAM.ERASE  1 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
set_property PROGRAM.CFG_PROGRAM  0 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
set_property PROGRAM.VERIFY  0 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
set_property PROGRAM.CHECKSUM  0 [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
set_property PROGRAM.ADDRESS_RANGE  {entire_device} [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
#set_property PROGRAM.BPI_RS_PINS {none} [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
#refresh_hw_device [lindex [get_hw_devices] 0]
startgroup 
create_hw_bitstream -hw_device [lindex [get_hw_devices ] 0] [get_property PROGRAM.HW_CFGMEM_BITFILE [ lindex [get_hw_devices ] 0]]; program_hw_devices [lindex [get_hw_devices ] 0]; refresh_hw_device [lindex [get_hw_devices ] 0];
program_hw_cfgmem -hw_cfgmem [ get_property PROGRAM.HW_CFGMEM [lindex [get_hw_devices] 0]]
endgroup

#boot_hw_device  [lindex [get_hw_devices] 0]
refresh_hw_device [lindex [get_hw_devices] 0]

close_hw_target

disconnect_hw_server
