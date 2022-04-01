# DriverEntry 
- Basic driver initialization routine when operating system first encounter our driver entry module IO Manager invoked Entry routine followed by PnP Manager to call into 
  our AddDevice routine.

# AddDevice
- For each instance of a device this adddevice get called once for every instance and a device object will get created. 
- We also register dispatch routine which this driver is about to handle when a particular 
  event occur.

# Dispatch Device Control:
- Here I try to handle "IOCTL_STORAGE_MANAGE_BYPASS_IO" in such a ammner that it will not break any windows 11 functionality. 
- In this IOCTL, I tried to fillin the desired input and output and send the request down to lower driver which will finally be handled by Storport/Miniport Driver.
- Note: No third party driver is currently supporting BypassIO in thier NVME Driver.



