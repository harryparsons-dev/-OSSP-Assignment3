cmd_/media/sf_VM_shared/ASS3/modules.order := {   echo /media/sf_VM_shared/ASS3/charDeviceDriver.ko; :; } | awk '!x[$$0]++' - > /media/sf_VM_shared/ASS3/modules.order
