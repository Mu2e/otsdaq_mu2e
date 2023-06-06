puts "JTAG index N: [lindex $argv 0]"
puts "JTAG-N bitfile to program: [lindex $argv 1]"

open_hw_manager
connect_hw_server

open_hw_target [lindex [get_hw_targets *] [lindex $argv 0]]
set_property PROGRAM.FILE [lindex $argv 1] [lindex [get_hw_devices] 0]
program_hw_devices [lindex [get_hw_devices] 0]
close_hw_target

disconnect_hw_server