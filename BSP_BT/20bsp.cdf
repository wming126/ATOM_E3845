/* 20bsp.cdf - BSP component description file */

/*
 * Copyright (c) 2010-2013 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01x,17sep13,scm  WIND00374998 - Add bundle for Bay Trail...
01w,28jan13,pee  WIND00400541 disable ROM resident build spec in VIP
01v,18dec12,yjw  Fix when remove one END driver, boot from net is 
                 removed. (WIND00380360) 
01u,26nov12,yjw  Add component GRUB_MULTIBOOT.(WIND00380360)
01t,24nov12,yjw  remove INCLUDE_SYS_WARM_FD and INCLUDE_SYS_WARM_TFFS
                 follow targte.ref.(WIND00380360)
01s,23nov12,yjw  Add Intel Norco support.
01r,08nov12,cfm  WIND00375416 - Added I8253 and MC146818 by default.
01q,22oct12,swu  WIND00382711 - Add SELECTION for INCLUDE_FAST_REBOOT and 
                 INCLUDE_MULTI_STAGE_WARM_REBOOT
01q,10jul12,wyt  WIND00353922 - Force unsupported component INCLUDE_ATA
01p,23may12,wyt  WIND00346005 - Use INCLUDE_SYMMETRIC_IO_MODE mode for
                 CrownBay and NITX.
01o,12may12,yjw  Add INCLUDE_SYS_WARM_USB no longer require USB GEN1
                 mass storage.(WIND00344759)
01n,20apr12,jjk  WIND00329365 - Support for SMP
01m,27mar12,wyt  WIND00340694 - Change to new AHCI and ICH component.
01l,28feb12,jjk  WIND00335210 - Make board selection selectable
01k,10jan12,sem  WIND00284000 - Remove I8253 exclusion.
                 WIND00322011 - Update APIC_TIMER_CLOCK_HZ.
                 WIND00327276 - Use selections for interrupt modes and
                 memory autosize.
01j,17nov11,wyt  Add components for Crownbay and NITX, add REQUIRES
                 for INCLUDE_BOOT_NET_DEVICES.
01i,15nov11,jjk  WIND00255693 - Adding support for Cedar Rock
01h,22apr11,jjk  Multi-stage boot support.
01g,28jan11,rbc  Additional changes for bundles.
01f,13jan11,mze  remove SMP as an option
01e,05jan11,rbc  WIND00244749 - Add support for bundles.
01d,03dec10,jb   Fix for WIND00242906 - USB config
01c,13nov10,mze  replace _WRS_VX_SMP with _WRS_CONFIG_SMP
01b,07oct10,sem  Update to new cpu tag
01a,30sep10,rbc  initial creation based on itl_nehalem version 01f
*/

Bsp BSP_BT {
    NAME        board support package
    CPU         ATOM
    REQUIRES    INCLUDE_KERNEL \
                INCLUDE_PCI \
                INCLUDE_PENTIUM_PCI \
                INCLUDE_PCI_OLD_CONFIG_ROUTINES \
                INCLUDE_ATOM_PARAMS \
                INCLUDE_TIMER_SYS \
                INCLUDE_TIMER_SYS_DELAY \
                INCLUDE_MMU_P6_32BIT \
                SELECT_INTERRUPT_MODE \
                SELECT_SYS_WARM_TYPE \
                SELECT_MEM_AUTOSIZE \
                SELECT_TARGET_BOARD \
                INCLUDE_BOARD_CONFIG \
                SELECT_USB_CONFIG \
                SELECT_USB_KB
    FP          hard
    MP_OPTIONS  UP SMP
    GUEST_OS_OPTION NOT_SUPPORTED
    BUILD_SPECS default_rom default_romCompress
}

/* ADLINK: Start
USB components are added to kernel module, as a default components */

Selection SELECT_USB_CONFIG {
    NAME        USB Keyboard selection
    COUNT       1-1
    CHILDREN    INCLUDE_USB_CONFIG
    DEFAULTS    INCLUDE_USB_CONFIG
    _CHILDREN   FOLDER_BSP_CONFIG    
}

Selection SELECT_USB_KB {
    NAME        USB devices selection
    COUNT       1-1
    CHILDREN    INCLUDE_USB_KB 
    _CHILDREN   FOLDER_BSP_CONFIG    
}

Component INCLUDE_USB_CONFIG {
    NAME        USB devices configuration component    
    REQUIRES    INCLUDE_USB \
                INCLUDE_USB_INIT \
                INCLUDE_EHCI \
                INCLUDE_EHCI_INIT \
                INCLUDE_UHCI \
                INCLUDE_UHCI_INIT \
                INCLUDE_USB_GEN2_STORAGE \
                INCLUDE_USB_GEN2_STORAGE_INIT \
                INCLUDE_DOSFS
}

Component INCLUDE_USB_KB {
    NAME        Including USB KB component    
    REQUIRES    INCLUDE_USB_GEN2_KEYBOARD \
                INCLUDE_USB_GEN2_KEYBOARD_INIT \
                INCLUDE_USB_GEN2_KEYBOARD_BOOTSHELL_ATTACH
}

/* ADLINK: End */


Component INCLUDE_BOARD_CONFIG {
    NAME        Board Configuration Component
    SYNOPSIS    Fundamental Board Configuration Component. Included by a Board Bundle
    REQUIRES    TARGET_NAME \
                DEFAULT_BOOT_LINE \
                SYS_MODEL
#if (defined _WRS_CONFIG_SMP)
    CFG_PARAMS  TARGET_NAME \
                DEFAULT_BOOT_LINE \
                SYS_AP_TIMEOUT \
                SYS_AP_LOOP_COUNT \
                SYS_MODEL
#else
    CFG_PARAMS  TARGET_NAME \
                DEFAULT_BOOT_LINE \
                SYS_MODEL
#endif /* _WRS_CONFIG_SMP */
    _INCLUDE_WHEN \
                DRV_TIMER_I8253 \
                DRV_TIMER_MC146818
}

Selection SELECT_TARGET_BOARD {
    NAME        Atom Target Selection
    SYNOPSIS    Selects Target Board for a build
    COUNT       1-1
    CHILDREN    INCLUDE_NANO \
                INCLUDE_WADE \
                INCLUDE_CROWNBEACH \
                INCLUDE_CEDAR_ROCK \
                INCLUDE_NAVYPIER \
                INCLUDE_MOONCREEK \
                INCLUDE_NITX \
                INCLUDE_CROWNBAY \
                INCLUDE_NORCO \
                INCLUDE_BAY_TRAIL
    DEFAULTS    INCLUDE_NANO
    _CHILDREN   FOLDER_BSP_CONFIG
}

Component INCLUDE_NANO {
    NAME        Nano Board Component
    SYNOPSIS    Provides Component Parameters for Nano Board
    REQUIRES    INCLUDE_BOARD_CONFIG
}

Component INCLUDE_WADE {
    NAME        Wade Board Component
    SYNOPSIS    Provides Component Parameters for Wade Board
    REQUIRES    INCLUDE_BOARD_CONFIG \
                INCLUDE_PIC_MODE \
                INCLUDE_RTL8169_VXB_END
}

Component INCLUDE_CROWNBEACH {
    NAME        Crownbeach Board Component
    SYNOPSIS    Provides Component Parameters for Crownbeach Board
    REQUIRES    INCLUDE_BOARD_CONFIG \
                INCLUDE_PIC_MODE \
                INCLUDE_PC_CONSOLE
}

Component INCLUDE_CEDAR_ROCK {
    NAME        Cedar Rock Board Component
    SYNOPSIS    Provides Component Parameters for Cedar Rock Board
    REQUIRES    INCLUDE_BOARD_CONFIG
}

Component INCLUDE_NAVYPIER {
    NAME        Navypier Board Component
    SYNOPSIS    Provides Component Parameters for Navypier Board
    REQUIRES    INCLUDE_BOARD_CONFIG \
                INCLUDE_PIC_MODE \
                INCLUDE_RTL8169_VXB_END
}

Component INCLUDE_MOONCREEK {
    NAME        Moon Creek Board Component
    SYNOPSIS    Provides Component Parameters for Moon Creek Board
    REQUIRES    INCLUDE_BOARD_CONFIG
}

Component INCLUDE_NITX {
    NAME        NITX Board Component
    SYNOPSIS    Provides Component Parameters for NITX Board
    REQUIRES    INCLUDE_BOARD_CONFIG
}

Component INCLUDE_CROWNBAY {
    NAME        Crownbay Board Component
    SYNOPSIS    Provides Component Parameters for Crownbay Board
    REQUIRES    INCLUDE_BOARD_CONFIG
}

Component INCLUDE_NORCO {
    NAME        NORCO Board Component
    SYNOPSIS    Provides Component Parameters for Norco Board
    REQUIRES    INCLUDE_BOARD_CONFIG
}

Component INCLUDE_BAY_TRAIL { 
    NAME        Bay Trail Board Component
    SYNOPSIS    Provides Component Parameters for Bay Trail Board
    REQUIRES    INCLUDE_BOARD_CONFIG
}                  
                   
Parameter TARGET_NAME {
    NAME     Atom Target
    SYNOPSIS Atom Target Board Name, leading space necessary.
    TYPE     string
    DEFAULT  (INCLUDE_MOONCREEK && INCLUDE_SMP_SCHED_SMT_POLICY)::("/SMT Mooncreek") \
             (INCLUDE_MOONCREEK)::(" Mooncreek") \
             (INCLUDE_CROWNBEACH)::(" Crownbeach") \
             (INCLUDE_NAVYPIER)::(" Navypier") \
             (INCLUDE_WADE)::(" Wade") \
             (INCLUDE_NANO)::(" Nano") \
             (INCLUDE_CEDAR_ROCK && INCLUDE_SMP_SCHED_SMT_POLICY)::("/SMT CedarRock") \
             (INCLUDE_CEDAR_ROCK)::(" CedarRock") \
             (INCLUDE_NITX)::(" NITX") \
             (INCLUDE_CROWNBAY)::(" Crownbay") \
             (INCLUDE_NORCO)::(" Norco") \
             (INCLUDE_BAY_TRAIL && INCLUDE_SMP_SCHED_SMT_POLICY)::("/SMT BayTrail") \
             (INCLUDE_BAY_TRAIL)::(" BayTrail") \
             ""
}

/* 
 * Network Boot Devices for a BSP.
 */
 
Component	INCLUDE_FEI8255X_VXB_END {
	INCLUDE_WHEN \
		+= INCLUDE_BOOT_NET_DEVICES
}
 	 
Component       INCLUDE_GEI825XX_VXB_END {
	INCLUDE_WHEN \
		+= INCLUDE_BOOT_NET_DEVICES
}
 	 
Component       DRV_VXBEND_INTELTOPCLIFF {
	INCLUDE_WHEN \
		+= INCLUDE_BOOT_NET_DEVICES
}

/* Specify boot rom console device for this BSP */
Component       INCLUDE_BOOT_SHELL {
    REQUIRES += DRV_SIO_NS16550
}

/* Filesystem Boot Devices for an Atom BSP
 * The REQUIRES line should be modified for a BSP.
 */
Component       INCLUDE_BOOT_FS_DEVICES_ATOM {
    REQUIRES	INCLUDE_BOOT_USB_FS_LOADER \
                INCLUDE_DOSFS \
                INCLUDE_USB \
                INCLUDE_USB_INIT \
                INCLUDE_EHCI \
                INCLUDE_EHCI_INIT \
                INCLUDE_UHCI \
                INCLUDE_UHCI_INIT
}

/* Filesystem Boot Devices for a Bay Trail Board
 * In order for USB attached file systems to fit in the
 * bootrom with ACPI included, BayTrail needs to use
 * the multi-stage boot mechanism.       
 */                                      
Component       INCLUDE_BOOT_FS_DEVICES_BAY_TRAIL {                                                 
    REQUIRES    INCLUDE_BOOT_FS_DEVICES_ATOM
    INCLUDE_WHEN INCLUDE_BAY_TRAIL INCLUDE_BOOT_APP INCLUDE_SYS_WARM_AHCI
}                                        

/* Delay time after USB Bulk Dev Init
*/
Parameter       BOOT_USB_POST_MS_BULKONLY_INIT_DELAY {
    NAME        Delay time after USB Bulk Dev Init
    SYNOPSIS    Amount of time in system ticks to delay after Bulk Init. \
                This allows the USB Task to run and discovery to occur \
                prior to attempting to crack the boot line.
    TYPE        int
    DEFAULT     (INCLUDE_CEDAR_ROCK && INCLUDE_BOOT_APP)::(3) \
                (INCLUDE_BAY_TRAIL && INCLUDE_BOOT_APP)::(3) \                                      
                (2)
}

/*                                    
*/                                    
Component       INCLUDE_SYS_WARM_AHCI_BAY_TRAIL {                                                    
    INCLUDES    INCLUDE_SYS_WARM_AHCI  
    INCLUDE_WHEN INCLUDE_BOOT_FS_DEVICES_BAY_TRAIL
}                                     

/*
 * Warm boot device selection required for
 * NVRAM support
 */

Selection SELECT_SYS_WARM_TYPE {
    NAME        Warm start device selection
    COUNT       1-1
    CHILDREN    INCLUDE_SYS_WARM_BIOS \
                INCLUDE_SYS_WARM_USB \
                INCLUDE_SYS_WARM_ICH \
                INCLUDE_SYS_WARM_AHCI
    DEFAULTS    INCLUDE_SYS_WARM_AHCI
    _CHILDREN   FOLDER_BSP_CONFIG
}

/*
 * Warm boot device components
 */

Component INCLUDE_SYS_WARM_BIOS {
    NAME        BIOS warm start device component
    CFG_PARAMS  SYS_WARM_TYPE \
                NV_RAM_SIZE
}

Component INCLUDE_SYS_WARM_USB {
    NAME        USB warm start device component
    REQUIRES    INCLUDE_USB \
                INCLUDE_USB_INIT \
                INCLUDE_EHCI \
                INCLUDE_EHCI_INIT \
                INCLUDE_DOSFS
    CFG_PARAMS  SYS_WARM_TYPE \
                NV_RAM_SIZE \
                BOOTROM_DIR
}


Component INCLUDE_SYS_WARM_ICH {
    NAME        ICH warm start device component
    REQUIRES    INCLUDE_DRV_STORAGE_PIIX \
                INCLUDE_DOSFS
    CFG_PARAMS  SYS_WARM_TYPE \
                NV_RAM_SIZE \
                BOOTROM_DIR
}

Component INCLUDE_SYS_WARM_AHCI {
    NAME        AHCI warm start device component
    REQUIRES    INCLUDE_DRV_STORAGE_AHCI \
                INCLUDE_DOSFS
    CFG_PARAMS  SYS_WARM_TYPE \
                NV_RAM_SIZE \
                BOOTROM_DIR
}

Parameter SYS_WARM_TYPE {
    NAME        Warm start device parameter
    DEFAULT     (INCLUDE_SYS_WARM_BIOS)::(SYS_WARM_BIOS) \
                (INCLUDE_SYS_WARM_USB)::(SYS_WARM_USB) \
                (INCLUDE_SYS_WARM_ICH)::(SYS_WARM_ICH) \
                (INCLUDE_SYS_WARM_AHCI)::(SYS_WARM_AHCI) \
                (SYS_WARM_BIOS)
}

Parameter NV_RAM_SIZE {
    DEFAULT     (INCLUDE_SYS_WARM_BIOS)::(NONE) \
                (0x1000)
}

Parameter BOOTROM_DIR {
    DEFAULT     (INCLUDE_SYS_WARM_USB)::("/bd0") \
                (INCLUDE_SYS_WARM_ICH)::("/ata0:1") \
                (INCLUDE_SYS_WARM_AHCI)::("/ata0:1") \
                (NULL)
}

/*
 * VX_SMP_NUM_CPUS is a SMP parameter only and only available for SMP
 * builds. Due to a limitation of the project tool at the time this
 * parameter is created where the tool can not recognize the ifdef SMP
 * selection, this parameter is set up such that _CFG_PARAMS is not
 * specified here. In the 00vxWorks.cdf file, where the parameter
 * VX_SMP_NUM_CPUS is defined, the _CFG_PARAMS is specified only for
 * VxWorks SMP. Hence the redefining of VX_SMP_NUM_CPUS here should only
 * override the value and not the rest of the properties. And for UP, the
 * parameter is ignored since the parameter is not tied to any component
 * (_CFG_PARAMS is not specified).
 */

Parameter VX_SMP_NUM_CPUS {
    NAME        Number of CPUs available to be enabled for VxWorks SMP
    TYPE        UINT
    DEFAULT     (INCLUDE_CEDAR_ROCK && INCLUDE_SMP_SCHED_SMT_POLICY)::(4) \
                (INCLUDE_CEDAR_ROCK)::(2) \
                (INCLUDE_MOONCREEK && INCLUDE_SMP_SCHED_SMT_POLICY)::(4) \
                (INCLUDE_MOONCREEK)::(2) \
                (INCLUDE_BAY_TRAIL && INCLUDE_SMP_SCHED_SMT_POLICY)::(4) \
                (INCLUDE_BAY_TRAIL)::(2) \
                (2)
}

Parameter ROM_TEXT_ADRS {
    NAME        ROM text address
    SYNOPSIS    ROM text address
    DEFAULT     (ROM_BASE_ADRS)
}

Parameter ROM_SIZE {
    NAME        ROM size
    SYNOPSIS    ROM size
    DEFAULT     (GRUB_MULTIBOOT)::(0x00200000) \
                (INCLUDE_MULTI_STAGE_BOOT)::(0x00200000) \
                (0x00090000)
}

Parameter ROM_BASE_ADRS {
    NAME        ROM base address
    SYNOPSIS    ROM base address
    DEFAULT     (GRUB_MULTIBOOT)::(0x00108000) \
                (INCLUDE_MULTI_STAGE_BOOT)::(0x00408000) \
                (0x00008000)
}

Component GRUB_MULTIBOOT {
    NAME        Grub multiboot component
    SYNOPSIS    Boot though grub multiboot
    _CHILDREN   FOLDER_BSP_CONFIG
    EXCLUDES    INCLUDE_MULTI_STAGE_BOOT \
                SELECT_MULTI_STAGE_REBOOT_TYPE
}

Component INCLUDE_MULTI_STAGE_BOOT {
    NAME        Multi-stage boot support
    SYNOPSIS    Use multi-stage fast or warm reboot mechanism
    REQUIRES    SELECT_MULTI_STAGE_REBOOT_TYPE \
                ROM_BASE_ADRS \
                ROM_TEXT_ADRS \
                ROM_SIZE
}

Selection SELECT_MULTI_STAGE_REBOOT_TYPE {
    NAME        Multi-stage boot type select
    COUNT       1-1
    CHILDREN    INCLUDE_FAST_REBOOT \
                INCLUDE_MULTI_STAGE_WARM_REBOOT
    DEFAULTS    INCLUDE_FAST_REBOOT
}

Component INCLUDE_FAST_REBOOT {
    NAME        Multi-stage fast boot type
    SYNOPSIS    Use multi-stage fast reboot mechanism
    REQUIRES    INCLUDE_MULTI_STAGE_BOOT
    EXCLUDES    INCLUDE_MULTI_STAGE_WARM_REBOOT \
                GRUB_MULTIBOOT
}

Component INCLUDE_MULTI_STAGE_WARM_REBOOT {
    NAME        Multi-stage warm boot type
    SYNOPSIS    Use multi-stage warm reboot mechanism
    REQUIRES    INCLUDE_MULTI_STAGE_BOOT
    EXCLUDES    INCLUDE_FAST_REBOOT \
                GRUB_MULTIBOOT
}

Parameter LOCAL_MEM_SIZE {
    NAME     system memory size
    DEFAULT  (INCLUDE_BOOT_APP)::(0x30000000) \
             (0x30000000)
}

Parameter SYS_MODEL {
    NAME     System Model
    SYNOPSIS System Model string
    TYPE     string
#if (!defined _WRS_CONFIG_SMP)
    DEFAULT  (INCLUDE_SYMMETRIC_IO_MODE)::("Intel(R) BayTrail SoC SYMMETRIC IO" TARGET_NAME) \
             (INCLUDE_VIRTUAL_WIRE_MODE)::("Intel(R) BayTrail SoC VIRTUAL WIRE" TARGET_NAME) \
             ("Intel(R) BayTrail SoC" TARGET_NAME)
#else
    DEFAULT  ("Intel(R) BayTrail SoC SYMMETRIC IO SMP" TARGET_NAME)
#endif
}

Parameter DEFAULT_BOOT_LINE {
    NAME     default boot line
    SYNOPSIS Default boot line string
    TYPE     string
    DEFAULT  ("fs(0,0)host:/bd0/vxWorks h=10.100.1.12 e=10.100.1.19:0xffffff00 u=user pw=user o=gei")
}

/*
 * SYS_AP_LOOP_COUNT is a SMP parameter used in function sysCpuStart.
 * It is used to set the count of times to check if an application
 * processor succeeded in starting up.
 */

Parameter SYS_AP_LOOP_COUNT {   
    NAME     System AP startup checks                                
    SYNOPSIS Maximum times to check if an application processor started
    TYPE     uint
    DEFAULT  (200000)
}

/*
 * SYS_AP_TIMEOUT is a SMP parameter used in function sysCpuStart.
 * It is used to set the time in microseconds to wait between checking
 * if an application processor succeeded in starting up.
 * The time is specified in microseconds and should be short in duration.
 * SYS_AP_LOOP_COUNT * SYS_AP_TIMEOUT gives the total time to wait for an AP
 * before giving up and moving on to the next application processor.
 */

Parameter SYS_AP_TIMEOUT {
    NAME     System AP startup timeout               
    SYNOPSIS Time between each check to see if application processor started
    TYPE     uint
    DEFAULT  (10)
}

/*******************************************************************************
*
* HWCONF component
*
*/
Component INCLUDE_HWCONF {
    NAME		hardware configuration
    SYNOPSIS	        hardware configuration support
    CONFIGLETTES	hwconf.c
    _CHILDREN	        FOLDER_NOT_VISIBLE
    _REQUIRES           INCLUDE_VXBUS
}

/*******************************************************************************
*
* When INCLUDE_DISABLE_LEGACY_NS16550_UART is defined, it allows the NS16550 driver
* to be included without including the legacy PC UART devices.  This is useful
* when the goal is to only support PCI based UART devices on x86.
*
*/

Component INCLUDE_DISABLE_LEGACY_NS16550_UART {
    NAME            Disables Legacy PC NS-16550 UART support
    SYNOPSIS        Disables Legacy UARTs as defined in hwconf.c
    _CHILDREN       FOLDER_BSP_CONFIG
    				
}
/*******************************************************************************
*
* Interrupt Mode Configuration
*
*/

Selection SELECT_INTERRUPT_MODE {
    NAME        Interrupt mode selection
    COUNT       1-1
    CHILDREN    INCLUDE_VIRTUAL_WIRE_MODE \
                INCLUDE_SYMMETRIC_IO_MODE \
                INCLUDE_PIC_MODE
    DEFAULTS    INCLUDE_SYMMETRIC_IO_MODE
    _REQUIRES   INCLUDE_VXBUS
    _CHILDREN   FOLDER_BSP_CONFIG
}

Component INCLUDE_VIRTUAL_WIRE_MODE {
    NAME        Virtual wire mode
    SYNOPSIS    Virtual wire interrupt mode
    REQUIRES    INCLUDE_INTCTLR_DYNAMIC_LIB \
                DRV_INTCTLR_MPAPIC \
                DRV_INTCTLR_LOAPIC \
                DRV_TIMER_LOAPIC \
                SELECT_MPAPIC_MODE \
                DRV_TIMER_I8253
    CFG_PARAMS  APIC_TIMER_CLOCK_HZ
}

Component INCLUDE_SYMMETRIC_IO_MODE {
    NAME        Symmetric I/O Mode
    SYNOPSIS    Symmetric I/O interrupt mode
    REQUIRES    INCLUDE_INTCTLR_DYNAMIC_LIB \
                DRV_INTCTLR_MPAPIC \
                DRV_INTCTLR_LOAPIC \
                DRV_TIMER_LOAPIC \
                SELECT_MPAPIC_MODE \
                DRV_INTCTLR_IOAPIC \
                DRV_TIMER_IA_TIMESTAMP
    CFG_PARAMS  APIC_TIMER_CLOCK_HZ
}

Component INCLUDE_PIC_MODE {
    NAME        PIC Mode
    SYNOPSIS    PIC interrupt mode
    REQUIRES    DRV_TIMER_I8253 \
                INCLUDE_NO_BOOT_OP
}

Parameter APIC_TIMER_CLOCK_HZ  {
    NAME        APIC timer clock rate configuration parameter
    SYNOPSIS    APIC timer clock rate (0 is auto-calculate)
    TYPE        uint
    DEFAULT     (INCLUDE_NORCO)::(133000000) \
                (0)
}

/*******************************************************************************
*
* MP Table Configuration Options
*
*/

Component INCLUDE_ACPI_MPAPIC {
    NAME        ACPI MP APIC
    SYNOPSIS    ACPI MP APIC component
    REQUIRES    INCLUDE_ACPI_BOOT_OP
}

Selection SELECT_MPAPIC_MODE {
    NAME        MP APIC boot options
    SYNOPSIS    Selects MP APIC struct creation method
    COUNT       1-1
    CHILDREN    INCLUDE_ACPI_BOOT_OP \
                INCLUDE_USR_BOOT_OP \
                INCLUDE_NO_BOOT_OP \
                INCLUDE_MPTABLE_BOOT_OP
    DEFAULTS    INCLUDE_MPTABLE_BOOT_OP
    _CHILDREN   FOLDER_BSP_CONFIG
}

Component INCLUDE_ACPI_BOOT_OP {
    NAME        ACPI MP APIC boot
    SYNOPSIS    ACPI MP APIC boot option
    REQUIRES    SELECT_INTERRUPT_MODE \
                INCLUDE_ACPI_MPAPIC \
                INCLUDE_ACPI_TABLE_MAP \
                INCLUDE_USR_MPAPIC
    EXCLUDES    INCLUDE_PIC_MODE
}

Component INCLUDE_USR_BOOT_OP {
    NAME        User defined MP APIC boot
    SYNOPSIS    User defined MP APIC option
    REQUIRES    SELECT_INTERRUPT_MODE
    EXCLUDES    INCLUDE_PIC_MODE
}

Component INCLUDE_MPTABLE_BOOT_OP {
    NAME        BIOS MP APIC boot
    SYNOPSIS    BIOS MP APIC boot option
    REQUIRES    SELECT_INTERRUPT_MODE
    EXCLUDES    INCLUDE_PIC_MODE
}

Component INCLUDE_NO_BOOT_OP {
    NAME        No MP APIC boot
    SYNOPSIS    No MP APIC boot option
    REQUIRES    SELECT_INTERRUPT_MODE
    EXCLUDES    INCLUDE_MPTABLE_BOOT_OP \
                INCLUDE_ACPI_BOOT_OP \
                INCLUDE_USR_BOOT_OP
}

/*
 * Force unsupported components to be unavailable.
 *
 * The following component definition(s) forces the named component(s)
 * to be "unavailable" as far as the project build facility (vxprj) is
 * concerned. The required component (COMPONENT_NOT_SUPPORTED) should
 * never be defined, and hence the named component(s) will never be
 * available. This little trick is used by the BSPVTS build scripts
 * (suiteBuild, bspBuildTest.tcl) to automatically exclude test modules
 * that are not applicable to a BSP because the BSP does not support a
 * given component and/or test module. If and when support is added,
 * the following definition(s) should be removed.
 */

Component INCLUDE_ATA {
    REQUIRES    COMPONENT_NOT_SUPPORTED
}
