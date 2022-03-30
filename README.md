# ByPassIo in Storage Drivers.

Starting in Windows 11,BypassIO was added as an optimized I/O path for reading from files. 
The goal of this new IO path is to reduce the CPU overhead of doing Direct IO reads, which helps to meet the I/O demands of loading and running next-generation games on Windows.BypassIO is a part of the infrastructure to support DirectStorage on Windows.
