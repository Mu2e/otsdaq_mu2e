for d in /sys/bus/usb/devices/*; do
  if [[ -f "$d/idVendor" && -f "$d/idProduct" ]]; then
    vendor=$(<"$d/idVendor")
    product=$(<"$d/idProduct")
    if [[ "$vendor" == "0403" && "$product" == "6014" ]]; then
      echo "Found Future Tech JTAG: $(basename "$d")"
    fi
    if [[ "$vendor" == "03fd" && "$product" == "0008" ]]; then
      echo "Found Xilinx II JTAG: $(basename "$d")"
    fi
  fi
done
echo "Then do, e.g... echo -n "1-5" > /sys/bus/usb/drivers/usb/unbind; sleep 1;"
echo "Then ...        echo -n "1-5" > /sys/bus/usb/drivers/usb/bind;"
