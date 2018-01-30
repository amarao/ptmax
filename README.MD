# PTMAX

A way to maximize a partition in a partition table to include it's trailing unallocated space.

## Introduction
Anyone who has tried to resize a partition with LVM's PV on it knew that feeling: what the hell, we are dealing with a 64-byte 35-years old fixed size structure with bunch of INTs in it and still we CAN NOT JUST RESIZE the partition.

All existing utilities look grim. Fdisk allows us to do this but with 'DELETE and CREATE' trick (if you miss numbers, you are fckd up), parted says it doesn't know how to do that with LVM partition, cloud-init can 'growpart' but for filesystem patitions only.

If we have an 'old plain spinning drive' with misarranged partitions, one can say: 'think before creating'. Because bare metal hard drives doesn't scale.

But now we are living in the world of virtual machines, iscsi, LVM, MD whose offer multitude of ways to 'change disk size in 2 clicks'. And what the hell you are supposed to do after the drive (block device) size has been changed but the partition table is not?

## Description
This utility partially solves that problem. It takes a partition name (/dev/sda1, /dev/xvda2 and so on) and attempts to maximize the partition. Only 4 bytes in the partition table (the 'size' field) are changed, nothing more. After that you can run pvreze, or resize2fs, or whatever, to reflect changes to a higher level.

Usage is pretty simple:

```
ptmax /dev/sda3
```

That's all.

This utility is very cautious to avoid doing bad things, so it will maximize a partition only if patition passed rigorous sanity checks and there is free space after the targeted partition. It will not move partitions, not attempt to use leading unpartitioned space, nor change the partition number, nor alter in any way the original partitioning schema, nor touch data inside any partition.

Ptmax supports only LBA only (which is default for modern paritions), so it wouldn't work on partiontions less then 32Mb in size (CHS style partitions).
