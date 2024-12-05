puts "Clearing vivado lab hw"

#vivado_lab -mode tcl
open_hw_manager
connect_hw_server
open_hw_target
close_hw_target
disconnect_hw_server
close_hw_manager

open_hw_manager
connect_hw_server
open_hw_target
close_hw_target
disconnect_hw_server
close_hw_manager

exit
