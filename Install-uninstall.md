## Installation:

#Steps:
- Right click on inf file and select the Install option, an operting system prompt will appear and ask you for reboot.
- Once you reboot the system. use devcon and check the driver installed successfully.
- Use this cmd in devcon to know the DiskDrive stack.
	devocn stack =DiskDrive

## UnInstallation:

#Steps:
- Open regedit in elevated mode.
- Goto "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\{4d36e967-e325-11ce-bfc1-08002be10318}".
- Check in LowerFilters Key you may see BypassFilter entry along with EhStorClass.
- Double click on LowerFilters Key and manually simply delet the entry of BypassFilter.
- Goto drivers and delete the driver filter of Bypassfilter.
- Reboot the system and again run the above devcon command.
- You should not be able to see the ByPassFilter in the diskdrivbe stack.

# NOTE:
- Devcon you can find in Winddk Samples provided by Microsoft.
