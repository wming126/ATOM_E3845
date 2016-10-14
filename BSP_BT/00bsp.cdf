/* 00bsp.cdf - BSP configuration file */

/*
 * Copyright (c) 2010-2012 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01n,22dec12,yjw  Fix set max/min aux clock rate failed in VTS.
                 (WIND00380360) 
01m,26nov12,yjw  Add component GRUB_MULTIBOOT.(WIND00380360)
01l,23nov12,yjw  Add Intel Norco support.(WIND00380360)
01k,22nov12,yjw  Undo WIND00273160.(WIND00380360)
01j,08nov12,cfm  WIND00375416 - Removed MC146818 as requirement in AUX CLK.
01i,22oct12,swu  WIND00382711 - Add SELECTION for INCLUDE_FAST_REBOOT and 
                 INCLUDE_MULTI_STAGE_WARM_REBOOT
01g,27mar12,wyt  WIND00340694 - Remove ATA related component.
01f,17nov11,jjk  WIND00255693 - Adding support for Cedar Rock
01e,12sep11,jjk  WIND00291992 - Set INCLUDE_PIIX4_ATADMA to child of FOLDER_HD
01d,22apr11,jjk  Multi-stage boot support.
01c,04nov10,sem  Remove INCLUDE_VX_CPUID_PROBE conditional (WIND00239845)
01b,25oct10,sem  Updates for new CPUID
01a,30sep10,rbc  initial creation based on itl_nehalem version 01d 
*/

/*
DESCRIPTION
This file overrides generic BSP components in comps/vxWorks/00bsp.cdf with
BSP_BT BSP-specific versions of components and parameters defined in the
generic CDF file.
*/


/*******************************************************************************
*
* Memory definitions
*
*/

Parameter LOCAL_MEM_LOCAL_ADRS {
    NAME        Physical memory base address
    DEFAULT     0x00100000
}

Parameter RAM_HIGH_ADRS {
    NAME        Bootrom Copy region
    DEFAULT     (INCLUDE_BOOT_APP)::(0x00608000) \
                 0x10008000
}

Parameter RAM_LOW_ADRS {
    NAME        Runtime kernel load address
    DEFAULT     (INCLUDE_BOOT_RAM_IMAGE)::(0x15008000) \
                (INCLUDE_BOOT_APP)::(0x10008000) \
                0x00408000
}

/*******************************************************************************
*
* System Clock, Auxiliary Clock and Timestamp Component and Parameters
*
*/

Component INCLUDE_TIMESTAMP {
#ifdef _WRS_CONFIG_SMP
    REQUIRES INCLUDE_HPET_TIMESTAMP
#else
    REQUIRES DRV_TIMER_IA_TIMESTAMP
#endif /* _WRS_CONFIG_SMP */
}

Parameter SYS_CLK_RATE_MAX  {
    NAME              system clock configuration parameter
    SYNOPSIS          maximum system clock rate
    TYPE              uint
    DEFAULT           (5000)
}

Parameter SYS_CLK_RATE_MIN  {
    NAME              system clock configuration parameter
    SYNOPSIS          minimum system clock rate
    TYPE              uint
    DEFAULT           (19)
}

Parameter SYS_CLK_RATE {
    NAME              system clock configuration parameter
    SYNOPSIS          number of ticks per second
    TYPE              uint
    DEFAULT           (60)
}

Component INCLUDE_AUX_CLK  {
    NAME              Auxiliary clock
    SYNOPSIS          Auxiliary clock component
    REQUIRES          INCLUDE_VXB_AUX_CLK
}

Parameter AUX_CLK_RATE_MAX  {
    NAME              auxiliary clock configuration parameter
    SYNOPSIS          maximum auxiliary clock rate
    TYPE              uint
    DEFAULT           (INCLUDE_PIC_MODE)::(8192) \
                      (5000)
}

Parameter AUX_CLK_RATE_MIN  {
    NAME              auxiliary clock configuration parameter
    SYNOPSIS          minimum auxiliary clock rate
    TYPE              uint
    DEFAULT           (INCLUDE_PIC_MODE)::(2) \
                      (19)
}

Parameter AUX_CLK_RATE  {
    NAME              auxiliary clock configuration parameter
    SYNOPSIS          default auxiliary clock rate
    TYPE              uint
    DEFAULT           (128)
}

Component INCLUDE_HPET_TIMESTAMP  {
    NAME              HPET timestamp
    SYNOPSIS          HPET timestamp component
    REQUIRES          DRV_TIMER_IA_HPET \
                      DRV_TIMER_IA_TIMESTAMP
}

/*******************************************************************************
*
* Cache Configuration Parameters
*
*/
Parameter USER_D_CACHE_MODE  {
    NAME              BSP_BT configuration parameter
    SYNOPSIS          write-back data cache mode
    TYPE              uint
    DEFAULT           (CACHE_COPYBACK | CACHE_SNOOP_ENABLE)
}

/*******************************************************************************
*
* Additional Intel Architecture show routines
*
*/
Component INCLUDE_INTEL_CPU_SHOW {
    NAME              Intel Architecture processor show routines
    SYNOPSIS          IA-32 processor show routines
    HDR_FILES         vxLib.h
    MODULES           vxShow.o
    INIT_RTN          vxShowInit ();
    _INIT_ORDER       usrShowInit
    _CHILDREN         FOLDER_SHOW_ROUTINES
    _DEFAULTS         += FOLDER_SHOW_ROUTINES
}

/*******************************************************************************
*
* BSP_BT BSP-specific configuration folder
*
*/
Folder FOLDER_BSP_CONFIG  {
    NAME              BSP_BT BSP configuration options
    SYNOPSIS          BSP-specific configuration
    CHILDREN          +=INCLUDE_ATOM_PARAMS \
                      INCLUDE_MULTI_STAGE_BOOT \
                      SELECT_MULTI_STAGE_REBOOT_TYPE \
                      INCLUDE_DEBUG_STORE \
                      GRUB_MULTIBOOT
    DEFAULTS          +=INCLUDE_ATOM_PARAMS
    _CHILDREN         FOLDER_HARDWARE
    _DEFAULTS         += FOLDER_HARDWARE
}

/*******************************************************************************
*
* BSP parameters Component
*
*/
Component INCLUDE_ATOM_PARAMS  {
    NAME              BSP build parameters
    SYNOPSIS          expose BSP configurable parameters
    LAYER             1
#ifdef _WRS_CONFIG_SMP
    CFG_PARAMS        INCLUDE_MTRR_GET    \
                      SYS_AP_LOOP_COUNT \
                      SYS_AP_TIMEOUT \
                      INCLUDE_PMC
#else
    CFG_PARAMS        INCLUDE_MTRR_GET    \
                      INCLUDE_PMC
#endif /* _WRS_CONFIG_SMP */
    HELP              BSP_BT
}

Parameter INCLUDE_MTRR_GET  {
    NAME              BSP_BT configuration parameter
    SYNOPSIS          get Memory Type Range Register settings from the BIOS
    TYPE              exists
    DEFAULT           TRUE
}

Parameter INCLUDE_PMC  {
    NAME              BSP_BT configuration parameter
    SYNOPSIS          Performance Monitoring Counter library support
    TYPE              exists
    DEFAULT           TRUE
}

/*******************************************************************************
*
* Physical Address Space Components
*
*/
Component INCLUDE_MMU_P6_32BIT  {
    NAME              32-bit physical address space
    SYNOPSIS          configure 32-bit address space support
    CFG_PARAMS        VM_PAGE_SIZE
    EXCLUDES          INCLUDE_MMU_P6_36BIT
    _CHILDREN         FOLDER_MMU
    _DEFAULTS         += FOLDER_MMU
    HELP              BSP_BT
}

Component INCLUDE_MMU_P6_36BIT  {
    NAME              36-bit physical address space extension
    SYNOPSIS          configure 36-bit address space extension support
    CFG_PARAMS        VM_PAGE_SIZE
    EXCLUDES          INCLUDE_MMU_P6_32BIT
    _CHILDREN         FOLDER_MMU
    HELP              BSP_BT
}

Parameter VM_PAGE_SIZE {
    NAME              VM page size
    SYNOPSIS          virtual memory page size (PAGE_SIZE_{4KB/2MB/4MB})
    TYPE              uint
    DEFAULT           PAGE_SIZE_4KB
}

/*******************************************************************************
*
* Debug Store BTS/PEBS Component and Parameters
*
*/
Component INCLUDE_DEBUG_STORE  {
    NAME              Debug Store BTS/PEBS support
    SYNOPSIS          configure Debug Store BTS/PEBS support
    CFG_PARAMS        DS_SYS_MODE \
                      BTS_ENABLED \
                      BTS_INT_MODE \
                      BTS_BUF_MODE \
                      PEBS_ENABLED \
                      PEBS_EVENT \
                      PEBS_METRIC \
                      PEBS_OS \
                      PEBS_RESET
    HELP              BSP_BT
}

Parameter DS_SYS_MODE {
    NAME              Debug Store BTS/PEBS operating mode
    SYNOPSIS          configure the Debug Store BTS/PEBS operating mode
    TYPE              bool
    DEFAULT           FALSE
}

Parameter BTS_ENABLED {
    NAME              enable or disable the BTS
    SYNOPSIS          enable or disable the BTS
    TYPE              bool
    DEFAULT           TRUE
}

Parameter BTS_INT_MODE {
    NAME              configure the BTS interrupt mode
    SYNOPSIS          configure the BTS interrupt mode
    TYPE              bool
    DEFAULT           TRUE
}

Parameter BTS_BUF_MODE {
    NAME              configure the BTS buffering mode
    SYNOPSIS          configure the BTS buffering mode
    TYPE              bool
    DEFAULT           TRUE
}

Parameter PEBS_ENABLED {
    NAME              enable or disable the PEBS
    SYNOPSIS          enable or disable the PEBS
    TYPE              bool
    DEFAULT           TRUE
}

Parameter PEBS_EVENT {
    NAME              specify the PEBS event
    SYNOPSIS          specify the PEBS event
    TYPE              uint
    DEFAULT           PEBS_FRONT_END
}

Parameter PEBS_METRIC {
    NAME              specify the PEBS metric
    SYNOPSIS          specify the PEBS metric
    TYPE              uint
    DEFAULT           PEBS_MEMORY_STORES
}

Parameter PEBS_OS {
    NAME              configure the PEBS execution mode
    SYNOPSIS          configure the PEBS execution mode
    TYPE              bool
    DEFAULT           TRUE
}

Parameter FTPS_INITIAL_DIR {
    NAME        FTP initial directory
    DEFAULT     "/ata0:1"
}

Parameter LOGIN_PASSWORD {
    NAME        rlogin/telnet encrypted password
    DEFAULT     "9FpXCuQt5y7yd8vAsumZT4Kl7TqDTFPycq7RpEuXtq4="
}

Parameter LOGIN_PASSWORD_SALT {
    NAME        rlogin/telnet encrypted password salt
    DEFAULT     "61EAAKdNAAA="
}

/*******************************************************************************
*
* MPAPIC initialization component
*
*/
Component INCLUDE_USR_MPAPIC {
    NAME                MPAPIC boot component
    SYNOPSIS            MPAPIC boot initialization support
    CONFIGLETTES        usrMpapic.c
    _CHILDREN           FOLDER_NOT_VISIBLE
    INCLUDE_WHEN        INCLUDE_VXBUS
}

