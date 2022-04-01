# Installation:

Steps:
1- Right click on inf file and select the Install option, the driver will install and ask operting system prompt you for reboot.
2- Once you reboot the system. use devcon and check the driver installed successfully.
3- Use this cmd in devcon to know the DiskDrive stack.
	devocn stack =DiskDrive

# UnInstallation:

Steps:
1- Open regedit in elevated mode.
2- Goto "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\{4d36e967-e325-11ce-bfc1-08002be10318}".
3- Check in LowerFilters Key you may see BypassFilter entry along with EhStorClass.
4- Double click on LowerFilters Key and manually simply delet the entry of BypassFilter.
5- Goto drivers and delete the driver filter of Bypassfilter.
6- Reboot the system and again run the above devcon command.
7- You should not be able to see the ByPassFilter in the diskdrivbe stack.

