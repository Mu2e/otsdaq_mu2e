puts "JTAG-0 bitfile to program: [lindex $argv 0]"
puts "JTAG-1 bitfile to program: [lindex $argv 1]"

open_hw_manager
connect_hw_server

open_hw_target [lindex [get_hw_targets *] 0]
set_property PROGRAM.FILE [lindex $argv 0] [lindex [get_hw_devices] 0]
program_hw_devices [lindex [get_hw_devices] 0]
close_hw_target

open_hw_target [lindex [get_hw_targets *] 1]
set_property PROGRAM.FILE [lindex $argv 1] [lindex [get_hw_devices] 0]
program_hw_devices [lindex [get_hw_devices] 0]
close_hw_target

disconnect_hw_server