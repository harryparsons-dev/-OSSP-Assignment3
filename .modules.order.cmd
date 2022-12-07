cmd_/media/sf_VM_shared/test/modules.order := {   echo /media/sf_VM_shared/test/charDeviceDriver.ko; :; } | awk '!x[$$0]++' - > /media/sf_VM_shared/test/modules.order
