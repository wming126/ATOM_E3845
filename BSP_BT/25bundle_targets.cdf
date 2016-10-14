/* 25bundle_targets.cdf - itl_atom BSP bundles for target boards */

/*
 * Copyright (c) 2011-2013 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01i,17sep13,scm  WIND00374998 - Add bundle for Bay Trail...
01h,28feb13,s_z  Add INCLUDE_BOOT_UHCI_RESET for Cedar Rock
                 and Norco bootapp bundle (WIND00390768)
01g,23nov12,yjw  Add Intel Norco support.(WIND00380360)
01f,31may12,wyt  WIND00346005 - Add INCLUDE_ACPI_BOOT_OP to 
                 NITX and Crownbay bundles.
01e,20apr12,jjk  WIND00329365 - Support for SMP
01d,17nov11,wyt  Add BUNDLEs for Crownbay and NITX-300/315.
01c,15nov11,jjk  WIND00255693 - Adding support for Cedar Rock
01b,22apr11,jjk  Multi-stage boot support.
01a,05jan11,rbc Created
*/

Bundle BUNDLE_BAY_TRAIL  {
    NAME        Intel Bay Trail Board bundle
    SYNOPSIS    Configures itl_atom vxWorks Image build for the Intel Bay Trail target.
    COMPONENTS  INCLUDE_BAY_TRAIL \
                INCLUDE_SYS_WARM_AHCI \
                DRV_PCI_SDHC_CTRL \
		DRV_MMCSTORAGE_CARD \
                INCLUDE_EHCI_INIT \
                INCLUDE_IPTELNETS
}

Bundle BUNDLE_BAY_TRAIL_SMT {
    NAME        Intel Bay Trail SMT Board bundle
    SYNOPSIS    Configures itl_atom vxWorks SMT Image build for the Intel Bay Trail target.
    COMPONENTS  INCLUDE_BAY_TRAIL \
                INCLUDE_SYS_WARM_AHCI \
                DRV_PCI_SDHC_CTRL \
		DRV_MMCSTORAGE_CARD \ 
                INCLUDE_SMP_SCHED_SMT_POLICY \
                INCLUDE_EHCI_INIT \
                INCLUDE_IPTELNETS
}

Bundle BUNDLE_BAY_TRAIL_BOOTAPP {
    NAME        Intel Bay Trail Bootapp Board bundle
    SYNOPSIS    Configures itl_atom BOOTAPP build for the Intel Bay Trail target.
    COMPONENTS  INCLUDE_BAY_TRAIL \
                INCLUDE_SYS_WARM_AHCI \
                DRV_PCI_SDHC_CTRL \
		DRV_MMCSTORAGE_CARD \
                INCLUDE_EHCI_INIT \
                INCLUDE_BOOT_UHCI_RESET
}

Bundle BUNDLE_MSB_FAST_REBOOT {
    NAME        Multi Stage Boot Fast Reboot bundle
    SYNOPSIS    Configures itl_atom BOOTAPP to do Fast reboots.
    COMPONENTS  INCLUDE_MULTI_STAGE_BOOT INCLUDE_FAST_REBOOT
}

Bundle BUNDLE_MSB_WARM_REBOOT {
    NAME        Multi Stage Boot Warm Reboot bundle
    SYNOPSIS    Configures itl_atom BOOTAPP to do warm reboots.
    COMPONENTS  INCLUDE_MULTI_STAGE_BOOT INCLUDE_MULTI_STAGE_WARM_REBOOT
}
