## Installation:

#Steps:
- Right click on inf file and select the Install option, an operting system prompt will appear and ask you for reboot.
- After rebooting the system. you can use devcon tool and check the driver stack of installed device class.
- Use the following command to check the device stack (of any class).
  devocn stack =DiskDrive

## UnInstallation:

#Steps:
- Open registry in Administartor mode.
- Goto "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\{4d36e967-e325-11ce-bfc1-08002be10318}".
- Check in LowerFilters Key you may see "DriverName" in our case it's BypassFilter, entry along with EhStorClass, if present.
- Double click on LowerFilters Key and manually delete the entry of DriverName.
- Goto drivers directory and delete the driver's sys file.
- Reboot the system and again run the  devcon command.
- You woult not be able to see the ByPassFilter in the diskdrivbe stack.

# NOTE:
- Devcon you can find in Winddk Samples provided by Microsoft.
