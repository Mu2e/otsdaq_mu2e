
open_hw_manager
connect_hw_server


open_hw_target [lindex [get_hw_targets *] 0]
boot_hw_device  [lindex [get_hw_devices] 0]
refresh_hw_device [lindex [get_hw_devices] 0]

close_hw_target

open_hw_target [lindex [get_hw_targets *] 1]
boot_hw_device  [lindex [get_hw_devices] 0]
refresh_hw_device [lindex [get_hw_devices] 0]

close_hw_target

disconnect_hw_server
