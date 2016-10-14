/* sysLib.c - itl_atom system-dependent library */

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
02a,20dec13,jb   VXW6-915 - System Mode Debug when printf is used to serial
                 console hang the printf
01z,07oct13,jb   WIND00437760 - Warm reboot fails if USER_RESERVED_MEM
                 configured
01y,17sep13,scm  WIND00374998 - Add bundle for Bay Trail...
01x,19jul13,jjk  WIND00427114 - Fixing prevent defects
01w,20mar13,scm  WIND00229494 - SMP dedicated segment register
01v,20mar13,s_z  WIND00390768 - Remove unused routine with the integration
                 of INCLUDE_BOOT_UHCI_RESET component
01u,26dec12,yjw  IRQ7 is not spurious interrupt in VIRTUAL_WIRE_MODE
                 (WIND00380360)
01t,20dec12,yjw  Fix BIOS leagcy UHCI issue.(WIND00380360)
01s,25nov12,yjw  Fix cold reboot failed on SCH chipset.(WIND00380360)
01r,24nov12,yjw  Ignore small memory block in E820 to reduce page table size.
                 (WIND00380360)
01q,15nov12,yjw  WIND00387850 - Fix exception due to NULL pPciDev in
                 sysDeviceFilter
01p,15aug12,c_t  WIND00334774 - Fix sysIntInitPIC() description
                 WIND00339581 - Fix sysIntEoiGet() description
01o,20apr12,jjk  WIND00329365 - Support for SMP
01n,21may12,c_t  rewrite sysUsDelay, add CPU frequency detect.(WIND00338455)
01m,09may12,j_l  WIND00337546 - Remove vxGdtrSet() declaration
01l,27mar12,wyt  WIND00340694 - Remove ATA driver related code.
01k,09jan12,sem  WIND00317771 - Handle unmatched interrupt vector in
                 sysIntEoiGet
01j,16jun11,jjk  Move Multi-Stage bootrom copy out of sysPhysMemTop and let
                 sysMemTop know about the size reserved for the copied bootrom.
01i,22apr11,jjk  Multi-stage boot support.
01h,28jan11,rbc  Fix apigen errors.
01g,20jan11,rbc  WIND00244749 - Add support for bundles.
01f,07dec10,rbc  WIND00244742 - Update sysMmuMapAdd.
01e,04nov10,sem  Remove INCLUDE_VX_CPUID_PROBE conditional (WIND00239845)
01d,29oct10,sem  Updates for new CPUID
01c,25oct10,jjk  WIND00151872 - Fix warm reboot from interrupt context. Moved
                 from IA BSP syslib.c files to be globally handled in
                 vxbLoApicIntr.c
01b,20oct10,cwl  include ataDrv.h if INCLUDE_ATA is defined. (WIND00231397)
01a,30sep10,rbc  initial creation based on itl_nehalem version 01k
*/

/*
DESCRIPTION
This library provides board-specific routines.  The device configuration
modules and device drivers included are:

    i8253Timer.c - Intel 8253 timer driver
    i8259Intr.c - Intel 8259 Programmable Interrupt Controller (PIC) library
    ioApicIntr.c - Intel IO APIC/xAPIC driver
    ioApicIntrShow.c - Intel IO APIC/xAPIC driver show routines
    iPiix4Pci.c - low level initalization code for PCI ISA/IDE Xcelerator
    loApicIntr.c - Intel Pentium[234] Local APIC/xAPIC driver
    loApicIntrShow.c - Intel Local APIC/xAPIC driver show routines
    loApicTimer.c - Intel Pentium2/3/4 Local APIC timer library
    sysNvRam.c - block device NVRAM library
    pccardLib.c - PC CARD enabler library
    pccardShow.c - PC CARD show library
    pciCfgStub.c - customizes pciConfigLib for the BSP
    pciCfgIntStub.c - customizes pciIntLib for the BSP
    pciConfigLib.c - PCI configuration space access support for PCI drivers
    pciIntLib.c - PCI shared interrupt support
    pciConfigShow.c - Show routines for PCI configuration library
    sysWindML.c - WindML BSP support routines


INCLUDE FILES: sysLib.h

SEE ALSO:
.pG "Configuration"
*/

/* includes (header file) */

#include <vxWorks.h>
#include <vsbConfig.h>
#include <vme.h>
#include <memLib.h>
#include <sysLib.h>
#include <string.h>
#include <intLib.h>
#include <config.h>
#include <logLib.h>
#include <taskLib.h>
#include <vxLib.h>
#include <errnoLib.h>
#include <dosFsLib.h>
#include <stdio.h>
#include <cacheLib.h>
#include <vmLib.h>
#include <arch/i86/ipiI86Lib.h>
#include <arch/i86/pentiumLib.h>

#if defined (INCLUDE_VIRTUAL_WIRE_MODE) || defined (INCLUDE_SYMMETRIC_IO_MODE)
#include <hwif/intCtlr/vxbMpApic.h>
#endif /* INCLUDE_VIRTUAL_WIRE_MODE || INCLUDE_SYMMETRIC_IO_MODE */
#include <hwif/fw/acpiLib.h>

#if defined (_WRS_CONFIG_SMP)
#include <private/kernelLibP.h>
#include <private/windLibP.h>
#include <private/vxSmpP.h>
#include <arch/i86/regsI86.h>
#include <fppLib.h>
#include <cpuset.h>
#include <vxCpuLib.h>
#include <vxIpiLib.h>
#endif /* (_WRS_CONFIG_SMP) */

#include <arch/i86/vxCpuArchLib.h>

#include <vxBusLib.h>
#ifdef  INCLUDE_TFFS
#   include <tffs/tffsDrv.h>
#endif  /* INCLUDE_TFFS */

#if defined (_WRS_CONFIG_SMP)
#if (!defined(INCLUDE_SYMMETRIC_IO_MODE))
#error "For the SMP BSP one must define INCLUDE_SYMMETRIC_IO_MODE !!!"
#endif
#endif /* (_WRS_CONFIG_SMP) */

/* defines */

#define ROM_SIGNATURE_SIZE  16

#if (VM_PAGE_SIZE == PAGE_SIZE_4KB)
#   if  (LOCAL_MEM_LOCAL_ADRS >= 0x00100000)
#       define LOCAL_MEM_SIZE_OS    0x00180000  /* n * VM_PAGE_SIZE */
#   else
#       define LOCAL_MEM_SIZE_OS    0x00080000  /* n * VM_PAGE_SIZE */
#   endif /* (LOCAL_MEM_LOCAL_ADRS >= 0x00100000) */
#else   /* VM_PAGE_SIZE is 2/4MB */
#   define LOCAL_MEM_SIZE_OS        VM_PAGE_SIZE    /* n * VM_PAGE_SIZE */
#endif  /* (VM_PAGE_SIZE == PAGE_SIZE_4KB) */

/*
 * IA-32 protected mode physical address space 4GB (2^32) and protected
 * mode physical address space extension 64GB (2^36) size constants.
 */

#define SIZE_ADDR_SPACE_32   (0x100000000ull)
#define SIZE_ADDR_SPACE_36   (0x1000000000ull)

/* maximum address space probed when using memory auto-sizing */

#define PHYS_MEM_MAX         (SIZE_ADDR_SPACE_32)

#define HALF(x)              (((x) + 1) >> 1)

/* sysPhysMemTop() memory test patterns */

#define TEST_PATTERN_A       (0x57696E64)
#define TEST_PATTERN_B       (0x52697665)
#define TEST_PATTERN_C       (0x72537973)
#define TEST_PATTERN_D       (0x74656D73)

/* Extended BIOS Data Area */

#ifndef EBDA_START
#define EBDA_START              0x9d000
#endif /* !EBDA_START */

/* imports */

#if defined (INCLUDE_VIRTUAL_WIRE_MODE) || defined (INCLUDE_SYMMETRIC_IO_MODE)
IMPORT MP_APIC_DATA  *pMpApicData;

#ifdef INCLUDE_USR_MPAPIC
IMPORT STATUS usrMpapicInit (BOOL earlyInit, char * pBuf);
#endif /* INCLUDE_USR_MPAPIC */

#endif /* INCLUDE_VIRTUAL_WIRE_MODE || INCLUDE_SYMMETRIC_IO_MODE */

#if defined(INCLUDE_SYMMETRIC_IO_MODE)
/*
 * These globals (glbLoApicOldSvr, glbLoApicOldLint0, glbLoApicOldLint1)
 * are used in sysLib.c, they avoid calling through vxbus and reduces
 * overhead, potential spinLock nesting...
 */

IMPORT UINT32 glbLoApicOldSvr;        /* original SVR */
IMPORT UINT32 glbLoApicOldLint0;      /* original LINT0 */
IMPORT UINT32 glbLoApicOldLint1;      /* original LINT1 */
#endif /* INCLUDE_SYMMETRIC_IO_MODE */

#if defined (_WRS_CONFIG_SMP)
IMPORT cpuset_t vxCpuEnabled;

IMPORT TSS      sysTss;
#endif /* (_WRS_CONFIG_SMP) */

#ifndef _WRS_CONFIG_SMP
IMPORT int         intCnt;                    /* interrupt mode counter */
#endif /* !_WRS_CONFIG_SMP */

IMPORT char        end;                       /* linker defined end-of-image */
IMPORT GDT         sysGdt[];                  /* the global descriptor table */
IMPORT void        elcdetach (int unit);
IMPORT VOIDFUNCPTR intEoiGet;                 /* BOI/EOI function pointer */
IMPORT void        intEnt (void);

IMPORT void        sysInitGDT (void);

IMPORT void        sysConfInit0 (void);
IMPORT void        sysConfInit (void);
IMPORT void        sysConfStop (void);

#if defined (_WRS_CONFIG_SMP)
IMPORT void sysInit(void);
IMPORT void sysInitCpuStartup(void);

#if defined (INCLUDE_RTP)
IMPORT STATUS syscallArchInit (void);
#endif /* INCLUDE_RTP */

#if defined INCLUDE_PROTECT_TASK_STACK ||       \
    defined INCLUDE_PROTECT_INTERRUPT_STACK
#if defined(_WRS_OSM_INIT)
IMPORT STATUS excOsmInit (UINT32, UINT32);
#endif /* defined(_WRS_OSM_INIT) */
#endif  /* INCLUDE_PROTECT_TASK_STACK || INCLUDE_PROTECT_INTERRUPT_STACK */

#endif /* (_WRS_CONFIG_SMP) */

#ifdef INCLUDE_VXBUS
#ifdef INCLUDE_SIO_UTILS
IMPORT void    sysSerialConnectAll(void);
#endif /* INCLUDE_SIO_UTILS */

IMPORT void (*_vxb_usDelayRtn)(int  delayTime);
IMPORT void    hardWareInterFaceInit();
IMPORT FUNCPTR sysIntLvlEnableRtn;
IMPORT FUNCPTR sysIntLvlDisableRtn;
STATUS sysIntEnablePIC (int irqNo);
STATUS sysIntDisablePIC (int irqNo);
#endif /* INCLUDE_VXBUS */

#ifdef INCLUDE_BCM43XX_END
IMPORT void sysBroadComWLBusInit(void);
#endif /* INCLUDE_BCM43XX_END */

#if defined (INCLUDE_MMU_P6_32BIT)
IMPORT STATUS mmuPro32Enable (BOOL enable);   /* disable MMU */
#else
IMPORT STATUS mmuPro36Enable (BOOL enable);   /* disable MMU */
#endif /* defined (INCLUDE_MMU_P6_32BIT) */

/* globals */

#ifdef PORT80_LED_DEBUG
void sysLedOut(short val);                                                                          
#endif /* PORT80_LED_DEBUG */

#ifdef INCLUDE_EARLY_PRINT
IMPORT void earlyPrint (char * outString);
#endif /* INCLUDE_EARLY_PRINT */

IMPORT void acpiIntrIntHelper (void);

#ifdef INCLUDE_CPU_PWR_ARCH

#ifndef INCLUDE_ACPI_CPU_CONFIG
/*
 * ACPI configuration tables.  Used as static tables when ACPI isn't included.
 *
 * Information contained in this table would be considered proprietary by
 * Intel.  ACPI can be used to construct the table by executing
 * sysBoardInfoPrint in a target shell using a vxWorks image created
 * with the INCLUDE_ACPI_CPU_CONFIG component.
 */

ACPI_BOARD_INFO  sysBspBoardInfo =
    {
    0,              /* acpiNumCpus */
    NULL,           /* pAcpiCpu */
    0,              /* acpiNumThermZones */
    NULL,           /* pThermalZone */
    };
ACPI_BOARD_INFO  *pSysBoardInfo = &sysBspBoardInfo;  /* static table */
#else
ACPI_BOARD_INFO  *pSysBoardInfo = &sysAcpiBoardInfo; /* constructed by ACPI */
#endif /* INCLUDE_ACPI_CPU_CONFIG */
#else /* INCLUDE_CPU_PERFORMANCE_MGMT */
ACPI_BOARD_INFO  *pSysBoardInfo = NULL;

#endif /* INCLUDE_CPU_PWR_ARCH */

PHYS_MEM_DESC sysPhysMemDesc [] =
    {

    /* adrs and length parameters must be page-aligned (multiples of 4KB) */

    /*
     * Any code access to the invalid address range will generate
     * a MMU exception and the offending task will be suspended.
     * Then use l(), lkAddr, ti(), and tt() to find the NULL access.
     */

    /* lower memory for invalid access */

    {
    (VIRT_ADDR) 0x0,
    (PHYS_ADDR) 0x0,
    _WRS_BSP_VM_PAGE_OFFSET,
    VM_STATE_MASK_FOR_ALL,
    VM_STATE_FOR_MEM_OS
    },

    /* lower memory for valid access */

    {
    (VIRT_ADDR) _WRS_BSP_VM_PAGE_OFFSET,
    (PHYS_ADDR) _WRS_BSP_VM_PAGE_OFFSET,
    0xa0000 - _WRS_BSP_VM_PAGE_OFFSET,
    VM_STATE_MASK_FOR_ALL,
    VM_STATE_FOR_MEM_OS
    },

    /* video ram, etc */

    {
    (VIRT_ADDR) 0x000a0000,
    (PHYS_ADDR) 0x000a0000,
    0x00060000,
    VM_STATE_MASK_FOR_ALL,
    VM_STATE_FOR_IO
    },

    /* upper memory for OS */

    {
    (VIRT_ADDR) LOCAL_MEM_LOCAL_ADRS,
    (PHYS_ADDR) LOCAL_MEM_LOCAL_ADRS,
    LOCAL_MEM_SIZE_OS,
    VM_STATE_MASK_FOR_ALL,
    VM_STATE_FOR_MEM_OS
    },

    /* upper memory for Application */

    {
    (VIRT_ADDR) LOCAL_MEM_LOCAL_ADRS + LOCAL_MEM_SIZE_OS,
    (PHYS_ADDR) LOCAL_MEM_LOCAL_ADRS + LOCAL_MEM_SIZE_OS,
    LOCAL_MEM_SIZE - LOCAL_MEM_SIZE_OS, /* it is changed in sysMemTop() */
    VM_STATE_MASK_FOR_ALL,
    VM_STATE_FOR_MEM_APPLICATION
    },

#   if defined(INCLUDE_SM_NET) && (SM_MEM_ADRS != 0x0)

    /* upper memory for sm net/obj pool */

    {
    (VIRT_ADDR) SM_MEM_ADRS,
    (PHYS_ADDR) SM_MEM_ADRS,
    SM_MEM_SIZE + SM_OBJ_MEM_SIZE,
    VM_STATE_MASK_FOR_ALL,
    VM_STATE_FOR_MEM_APPLICATION
    },

#   endif /* defined(INCLUDE_SM_NET) && (SM_MEM_ADRS != 0x0) */

    /* entries for dynamic mappings - create sufficient entries */

    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY,
    DUMMY_MMU_ENTRY
    };

int sysPhysMemDescNumEnt;   /* number Mmu entries to be mapped */

#ifndef INCLUDE_VXBUS
#ifdef  INCLUDE_PC_CONSOLE

PC_CON_DEV  pcConDv [N_VIRTUAL_CONSOLES] =
    {
    {{{{NULL}}}, FALSE, NULL, NULL},
    {{{{NULL}}}, FALSE, NULL, NULL}
    };

#endif  /* INCLUDE_PC_CONSOLE */
#endif /* INCLUDE_VXBUS */

#ifdef INCLUDE_FD

IMPORT STATUS usrFdConfig (int type, int drive, char *fileName);
FD_TYPE fdTypes[] =
    {
    {2880,18,2,80,2,0x1b,0x54,0x00,0x0c,0x0f,0x02,1,1,"1.44M"},
    {2400,15,2,80,2,0x24,0x50,0x00,0x0d,0x0f,0x02,1,1,"1.2M"},
    };
UINT    sysFdBuf     = FD_DMA_BUF_ADDR; /* floppy disk DMA buffer address */
UINT    sysFdBufSize = FD_DMA_BUF_SIZE; /* floppy disk DMA buffer size */

#endif  /* INCLUDE_FD */

#ifdef  INCLUDE_LPT

LPT_RESOURCE lptResources [N_LPT_CHANNELS] =
    {
    {LPT0_BASE_ADRS, INT_NUM_LPT0, LPT0_INT_LVL,
    TRUE, 10000, 10000, 1, 1, 0
    },

    {LPT1_BASE_ADRS, INT_NUM_LPT1, LPT1_INT_LVL,
    TRUE, 10000, 10000, 1, 1, 0
    },

    {LPT2_BASE_ADRS, INT_NUM_LPT2, LPT2_INT_LVL,
    TRUE, 10000, 10000, 1, 1, 0
    }
    };

#endif  /* INCLUDE_LPT */

int     sysBus = BUS;      /* system bus type (VME_BUS, etc) */
int     sysCpu = CPU;      /* system cpu type (MC680x0) */

char    *sysBootLine = BOOT_LINE_ADRS; /* address of boot line */
char    *sysExcMsg   = EXC_MSG_ADRS; /* catastrophic message area */

int     sysProcNum;         /* processor number of this cpu */

UINT    sysIntIdtType   = SYS_INT_INTGATE;  /* IDT entry type */
UINT    sysProcessor    = (UINT) NONE;      /* 0=386, 1=486, 2=P5, 4=P6, 5=P7 */
UINT    sysCoprocessor  = 0;        /* 0=none, 1=387, 2=487 */

int     sysWarmType = SYS_WARM_TYPE;      /* system warm boot type */

UINT    sysStrayIntCount = 0;       /* Stray interrupt count */

char    *memTopPhys = NULL;     /* top of memory */

UINT32  memRom     = 0; /* saved bootrom image */
UINT32  memRomSize = 0; /* saved bootrom image size */


/* Initial GDT to be initialized */

GDT *pSysGdt    = (GDT *)(LOCAL_MEM_LOCAL_ADRS + GDT_BASE_OFFSET);

/*
 * mpCpuIndexTable is used in vxCpuArchLib.c, this global avoids
 * calling through vxbus and reduces overhead, table translations
 * are done everywhere, but once up this table should never change...
 */

UINT8 mpCpuIndexTable[256];

#ifdef _WRS_CONFIG_SMP
/* TSS initialized */

TSS *sysTssCurrent = &sysTss;                   /* current sysTss */

FUNCPTR sysCpuInitTable[VX_MAX_SMP_CPUS] = {NULL};
unsigned int sysCpuLoopCount[VX_MAX_SMP_CPUS] = {0};
#else
TSS      sysTss;
#endif /* _WRS_CONFIG_SMP */

#if defined (INCLUDE_SYMMETRIC_IO_MODE) || defined (INCLUDE_VIRTUAL_WIRE_MODE)
/*
 * These are used in sysLib.c to avoid calling out
 * through vxbus to reduces overhead, and potential
 * spinLock nesting...
 */

#if defined(INCLUDE_SYMMETRIC_IO_MODE)
LOCAL VXB_DEVICE_ID ioApicDevID;

#ifdef _WRS_CONFIG_SMP
LOCAL UINT8 tramp_code[CPU_AP_AREA];
#endif /* _WRS_CONFIG_SMP */
#endif /* INCLUDE_SYMMETRIC_IO_MODE */

LOCAL VXB_DEVICE_ID mpApicDevID;
LOCAL VXB_DEVICE_ID loApicDevID;

STATUS apicIntrIntHelper(VXB_DEVICE_ID pDev, void * unused);

LOCAL STATUS (*getMpApicLoIndexTable)(VXB_DEVICE_ID pInst,
                                      UINT8 ** mploApicIndexTable);
LOCAL STATUS (*getMpApicloBaseGet) (VXB_DEVICE_ID  pInst,
                                    UINT32 *mpApicloBase);

#if defined(INCLUDE_SYMMETRIC_IO_MODE)
LOCAL STATUS (*getMpApicNioApicGet) (VXB_DEVICE_ID  pInst,
                                     UINT32 *mpApicNioApic);

LOCAL STATUS (*getMpApicAddrTableGet) (VXB_DEVICE_ID  pInst,
                                       UINT32 **mpApicAddrTable);

LOCAL STATUS (*getIoApicRedNumEntriesGet)(VXB_DEVICE_ID pInst,
                                          UINT8 *ioApicRedNumEntries);

LOCAL STATUS (*getIoApicIntrIntEnable)(VXB_DEVICE_ID pInst,
                                       INT32 * irqNo);
LOCAL STATUS (*getIoApicIntrIntDisable)(VXB_DEVICE_ID pInst,
                                        INT32 * irqNo);

LOCAL STATUS (*getIoApicIntrIntLock)(VXB_DEVICE_ID pInst, int * dummy);
LOCAL STATUS (*getIoApicIntrIntUnlock)(VXB_DEVICE_ID pInst, int * dummy);
#endif /* INCLUDE_SYMMETRIC_IO_MODE */

LOCAL STATUS (*getLoApicIntrInitAP)(VXB_DEVICE_ID pInst, int *dummy);
LOCAL STATUS (*getLoApicIntrEnable)(VXB_DEVICE_ID pInst, BOOL *enable);

LOCAL STATUS (*getLoApicIntrIntLock)(VXB_DEVICE_ID pInst, int * dummy);
LOCAL STATUS (*getLoApicIntrIntUnlock)(VXB_DEVICE_ID pInst, int * dummy);

void sysStaticMpApicDataGet(MP_APIC_DATA *pMpApicData);
#endif /* defined (INCLUDE_SYMMETRIC_IO_MODE) || */
       /* defined (INCLUDE_VIRTUAL_WIRE_MODE)    */

#include <hwif/cpu/arch/i86/vxCpuIdLib.h>

IMPORT int vxCpuIdProbe(VX_CPUID *);

UINT vxProcessor = (UINT) NONE; /* 6=core, 7=atom, 8=nehalem, 9=sandybridge */

VX_CPUID vxCpuId = {{0,{0},0,0,0,0,0,0,0,0,{0,1},{0}}}; /* VX_CPUID struct. */

/* for backward compatibility, maintain old CPUID struct */

CPUID sysCpuId = {0,{0},0,0,0,0,0,0,0,0,{0,1},{0}}; /* CPUID struct. */

#if !defined (_WRS_CONFIG_SMP)
BOOL    sysBp       = TRUE;     /* TRUE for BP, FALSE for AP */
#endif /* !(_WRS_CONFIG_SMP) */

#if defined (INCLUDE_VIRTUAL_WIRE_MODE)
UINT8   vwInumTbl[] =   /* IRQ vs IntNum table for INCLUDE_VIRTUAL_WIRE_MODE */
    {
    INT_NUM_IRQ0,               /* IRQ  0 Vector No */
    INT_NUM_IRQ0 + 1,           /* IRQ  1 Vector No */
    INT_NUM_IRQ0 + 2,           /* IRQ  2 Vector No */
    INT_NUM_IRQ0 + 3,           /* IRQ  3 Vector No */
    INT_NUM_IRQ0 + 4,           /* IRQ  4 Vector No */
    INT_NUM_IRQ0 + 5,           /* IRQ  5 Vector No */
    INT_NUM_IRQ0 + 6,           /* IRQ  6 Vector No */
    INT_NUM_IRQ0 + 7,           /* IRQ  7 Vector No */
    INT_NUM_IRQ0 + 8,           /* IRQ  8 Vector No */
    INT_NUM_IRQ0 + 9,           /* IRQ  9 Vector No */
    INT_NUM_IRQ0 + 10,          /* IRQ 10 Vector No */
    INT_NUM_IRQ0 + 11,          /* IRQ 11 Vector No */
    INT_NUM_IRQ0 + 12,          /* IRQ 12 Vector No */
    INT_NUM_IRQ0 + 13,          /* IRQ 13 Vector No */
    INT_NUM_IRQ0 + 14,          /* IRQ 14 Vector No */
    INT_NUM_IRQ0 + 15,          /* IRQ 15 Vector No */
    INT_NUM_LOAPIC_TIMER,       /* Local APIC Timer Vector No */
    INT_NUM_LOAPIC_ERROR,       /* Local APIC Error Vector No */
    INT_NUM_LOAPIC_LINT0,       /* Local APIC LINT0 Vector No */
    INT_NUM_LOAPIC_LINT1,       /* Local APIC LINT1 Vector No */
    INT_NUM_LOAPIC_PMC,         /* Local APIC PMC Vector No */
    INT_NUM_LOAPIC_THERMAL,     /* Local APIC Thermal Vector No */
    INT_NUM_LOAPIC_SPURIOUS,    /* Local APIC Spurious Vector No */
    INT_NUM_LOAPIC_SM,          /* Local APIC SM Vector No */
    INT_NUM_LOAPIC_SM + 1,      /* Local APIC SM Vector No */
    INT_NUM_LOAPIC_SM + 2,      /* Local APIC SM Vector No */
    INT_NUM_LOAPIC_SM + 3,      /* Local APIC SM Vector No */
    INT_NUM_LOAPIC_IPI,         /* Local APIC IPI Vector No */
    INT_NUM_LOAPIC_IPI + 1,     /* Local APIC IPI Vector No */
    INT_NUM_LOAPIC_IPI + 2,     /* Local APIC IPI Vector No */
    INT_NUM_LOAPIC_IPI + 3,     /* Local APIC IPI Vector No */
    INT_NUM_LOAPIC_IPI + 4,     /* Local APIC IPI Vector No */
    INT_NUM_LOAPIC_IPI + 5,     /* Local APIC IPI Vector No */
    INT_NUM_LOAPIC_IPI + 6,     /* Local APIC IPI Vector No */
    INT_NUM_LOAPIC_IPI + 7      /* Local APIC IPI Vector No */
    };
#endif /* defined (INCLUDE_VIRTUAL_WIRE_MODE) */

#if defined(INCLUDE_SYMMETRIC_IO_MODE)
/*
 * Under INCLUDE_SYMMETRIC_IO_MODE sysInumTbl is created dynamically under the
 * IO Apic driver initialization.
 *
 * Multiple IO Apics are used in high-end servers to distribute irq load.
 * At this time there may be as many as 8 IO Apics in a system. The most common
 * number of inputs on a single IO Apic is 24, but they are also designed for
 * 16/32/or 64 inputs.
 *
 * The common case here is a 1:1 irq<=>pin mapping.
 *
 * The mapping is critical to system operation. We need to support the first
 * 16 interrupts as legacy ISA/EISA.
 *
 * irq 0,1,2,3,4,6,8,12,13,14, and 15 are typically consumed by legacy devices.
 *
 * irq 5,7,9,10, and 11 are typically available for general use (though 5 is
 * sometimes used for audio devices and 7 is used for LPT).
 *
 * 16,17,18, and 19 are typically used for standard PCI.
 *
 * Each of the interrupts can be used programmed with any of the 224 vectors the
 * CPU is capable of. The first IO Apic will handle the ISA/EISA interrupts.
 *
 * The way this will work: if (irq <= 16) just map the irq to whatever pin is
 * requested. If (irq > 16) we enforce the mapping irq =
 * (apic*num_ioapic_entriess)+pin.
 *
 * This means that given any irq we can always derive the apic and pin.
 *
 *          apic from irq => irq / num_ioapic_entriess
 *
 *          pin from irq  => irq % num_ioapic_entriess
 */

#endif /* INCLUDE_SYMMETRIC_IO_MODE */

UINT8   dfltInumTbl[] = /* IRQ vs IntNum table for default interrupt handling */
    {
    INT_NUM_IRQ0,               /* IRQ  0 Vector No */
    INT_NUM_IRQ0 + 1,           /* IRQ  1 Vector No */
    INT_NUM_IRQ0 + 2,           /* IRQ  2 Vector No */
    INT_NUM_IRQ0 + 3,           /* IRQ  3 Vector No */
    INT_NUM_IRQ0 + 4,           /* IRQ  4 Vector No */
    INT_NUM_IRQ0 + 5,           /* IRQ  5 Vector No */
    INT_NUM_IRQ0 + 6,           /* IRQ  6 Vector No */
    INT_NUM_IRQ0 + 7,           /* IRQ  7 Vector No */
    INT_NUM_IRQ0 + 8,           /* IRQ  8 Vector No */
    INT_NUM_IRQ0 + 9,           /* IRQ  9 Vector No */
    INT_NUM_IRQ0 + 10,          /* IRQ 10 Vector No */
    INT_NUM_IRQ0 + 11,          /* IRQ 11 Vector No */
    INT_NUM_IRQ0 + 12,          /* IRQ 12 Vector No */
    INT_NUM_IRQ0 + 13,          /* IRQ 13 Vector No */
    INT_NUM_IRQ0 + 14,          /* IRQ 14 Vector No */
    INT_NUM_IRQ0 + 15,          /* IRQ 15 Vector No */
    };

UINT8 *sysInumTbl;
UINT32 sysInumTblNumEnt;

/* locals */

#ifdef  INCLUDE_ROMCARD

LOCAL short *sysRomBase[] =
    {
    (short *)0xce000, (short *)0xce800, (short *)0xcf000, (short *)0xcf800
    };

LOCAL char sysRomSignature[ROM_SIGNATURE_SIZE] =
    {
    0x55,0xaa,0x01,0x90,0x90,0x90,0x90,0x90,
    0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90
    };

#endif  /* INCLUDE_ROMCARD */

    /*
     * The cache control flags and MTRRs operate hierarchically for restricting
     * caching.  That is, if the CD flag is set, caching is prevented globally.
     * If the CD flag is clear, either the PCD flags and/or the MTRRs can be
     * used to restrict caching.  If there is an overlap of page-level caching
     * control and MTRR caching control, the mechanism that prevents caching
     * has precedence.  For example, if an MTRR makes a region of system memory
     * uncachable, a PCD flag cannot be used to enable caching for a page in
     * that region.  The converse is also true; that is, if the PCD flag is
     * set, an MTRR cannot be used to make a region of system memory cacheable.
     * If there is an overlap in the assignment of the write-back and write-
     * through caching policies to a page and a region of memory, the write-
     * through policy takes precedence.  The write-combining policy takes
     * precedence over either write-through or write-back.
     */

LOCAL MTRR sysMtrr =
    {                   /* MTRR table */
    {0,0},              /* MTRR_CAP register */
    {0,0},              /* MTRR_DEFTYPE register */

                        /* Fixed Range MTRRs */

    {{{MTRR_WB, MTRR_WB, MTRR_WB, MTRR_WB, MTRR_WB, MTRR_WB, MTRR_WB, MTRR_WB}},
     {{MTRR_WB, MTRR_WB, MTRR_WB, MTRR_WB, MTRR_WB, MTRR_WB, MTRR_WB, MTRR_WB}},
     {{MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_WC, MTRR_WC, MTRR_WC, MTRR_WC}},
     {{MTRR_WP, MTRR_WP, MTRR_WP, MTRR_WP, MTRR_WP, MTRR_WP, MTRR_WP, MTRR_WP}},
     {{MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC}},
     {{MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC}},
     {{MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC}},
     {{MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC}},
     {{MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC}},
     {{MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC}},
     {{MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC, MTRR_UC}}},
    {{0LL, 0LL},            /* Variable Range MTRRs */
     {0LL, 0LL},
     {0LL, 0LL},
     {0LL, 0LL},
     {0LL, 0LL},
     {0LL, 0LL},
     {0LL, 0LL},
     {0LL, 0LL}}
    };

/* forward declarations */

void sysStrayInt   (void);
char * sysPhysMemTop     (void);
STATUS sysMmuMapAdd  (void * address, UINT len, UINT initialStateMask,
                      UINT initialState);

void sysIntInitPIC (void);
void sysIntEoiGet (VOIDFUNCPTR * vector,
                   VOIDFUNCPTR * routineBoi, int * parameterBoi,
                   VOIDFUNCPTR * routineEoi, int * parameterEoi);

void sysUsDelay (int uSec);

void sysHardReset(void);

STATUS sysHpetOpen(void);

#if defined (_WRS_CONFIG_SMP)
void sysCpuInit (void);

UINT32 sysCpuAvailableGet(void);

STATUS sysCpuStart(int cpuNum, WIND_CPU_STATE *cpuState);
STATUS vxCpuStateInit(unsigned int cpu, WIND_CPU_STATE *cpuState,
                      char *stackBase, FUNCPTR entry);
STATUS sysCpuEnable(unsigned int cpuNum, WIND_CPU_STATE *cpuState);
STATUS sysCpuDisable(int cpuNum);

STATUS sysCpuStop(int cpu);
STATUS sysCpuStop_APs(void);
STATUS sysCpuStop_ABM(void);

STATUS sysCpuReset(int cpu);
STATUS sysCpuReset_APs(void);
STATUS sysCpuReset_ABM(void);
#endif /* (_WRS_CONFIG_SMP) */

#if !defined (INCLUDE_SYMMETRIC_IO_MODE) || !defined (INCLUDE_VIRTUAL_WIRE_MODE)
unsigned int dummyGetCpuIndex (void);
#endif /* !(INCLUDE_SYMMETRIC_IO_MODE) || !(INCLUDE_VIRTUAL_WIRE_MODE) */

/* includes (source file) */

#if (NV_RAM_SIZE != NONE)
#   include "sysNvRam.c"
#else   /* default to nullNvRam */
#   include <mem/nullNvRam.c>
#endif  /* (NV_RAM_SIZE != NONE) */

#ifndef DRV_SIO_NS16550

/* use vxBus ns16550 if INCLUDE_VXBUS is defined */

#include "sysSerial.c"
#endif /* DRV_SIO_NS16550 */

#include <vme/nullVme.c>

#if (!defined(INCLUDE_SYMMETRIC_IO_MODE))
#include <intrCtl/i8259Intr.c>
#endif /* !defined(INCLUDE_SYMMETRIC_IO_MODE) */

#ifdef  INCLUDE_PCI_BUS                     /* BSP PCI bus & config support */
#   include <drv/pci/pciConfigLib.h>
#   include <drv/pci/pciIntLib.h>
#   include <drv/pci/pciAutoConfigLib.h>

/* 24-bit PCI network class ethernet subclass and prog. I/F code */

#   include "pciCfgStub.c"
#   include "pciCfgIntStub.c"

#endif  /* INCLUDE_PCI_BUS */

#ifdef  INCLUDE_PCMCIA
#   include <pcmcia/pccardLib.c>
#   include <pcmcia/pccardShow.c>
#endif  /* INCLUDE_PCMCIA */

/* include BSP specific WindML configuration */

#if defined(INCLUDE_WINDML) && defined(INCLUDE_PCI_BUS)
#ifndef INCLUDE_VXBUS
#    include "sysWindML.c"
#endif /*  INCLUDE_VXBUS */
#endif /* INCLUDE_WINDML */

#ifdef  INCLUDE_DEBUG_STORE
#   include "sysDbgStr.c"       /* Debug Store support */
#endif  /* INCLUDE_DEBUG_STORE */

/* Atheros AR521X WLAN Support */

#ifdef INCLUDE_AR521X_END
    #include <wlan/bsp/sysDot11End.c>
#endif /* INCLUDE_AR521X_END */

#ifdef INCLUDE_BCM43XX_END
    #include <wlan/bsp/sysBroadComWLEnd.c>
#endif /* INCLUDE_BCM43XX_END */

#if defined INCLUDE_BIOS_E820_MEM_AUTOSIZE
#   include <hwif/fw/bios/vxBiosE820Lib.h>
#endif /* INCLUDE_BIOS_E820_MEM_AUTOSIZE */

#ifdef INCLUDE_VXBUS
#include <hwif/vxbus/vxBus.h>

IMPORT device_method_t * pSysPlbMethods;

#if defined(INCLUDE_PC_CONSOLE) && defined (DRV_TIMER_I8253)
LOCAL void sysConBeep (BOOL);
IMPORT char * pcConBeep_desc;
#endif /* INCLUDE_PC_CONSOLE && DRV_TIMER_I8253 */

LOCAL struct vxbDeviceMethod pc386PlbMethods[] =
    {
#if defined(INCLUDE_PC_CONSOLE) && defined (DRV_TIMER_I8253)
    DEVMETHOD(pcConBeep, sysConBeep),
#endif /* INCLUDE_PC_CONSOLE && DRV_TIMER_I8253 */
    { 0, 0 }
    };

#endif /* INCLUDE_VXBUS */

/******************************************************************************
*
* sysModel - return the model name of the CPU board
*
* This routine returns the model name of the CPU board.
*
* RETURNS: A pointer to the string "PC 386, 486, PENTIUM or PENTIUM[234]".
*/

char *sysModel (void)

    {
    return (SYS_MODEL);
    }

/*******************************************************************************
*
* sysBspRev - return the BSP version and revision number
*
* This routine returns a pointer to a BSP version and revision number, for
* example, 1.1/0. BSP_REV is concatenated to BSP_VERSION and returned.
*
* RETURNS: A pointer to the BSP version/revision string.
*/

char * sysBspRev (void)
    {
    return (BSP_VERSION BSP_REV);
    }



#ifdef INCLUDE_SYS_HW_INIT_0

/*******************************************************************************
*
* sysHwInit0 - BSP-specific hardware initialization
*
* This routine is called from usrInit() to perform BSP-specific initialization
* that must be done before cacheLibInit() is called and/or the BSS is cleared.
*
* The vxCpuIdProbe() routine is called for the purpose of
* identifying IA target CPU variants, and the features or functions
* supported by the target CPU.  This information must be obtained relatively
* early during system hardware initialization, as some support libraries
* (mmuLib, cacheLib, &c.) will use the processor feature information to
* enable or disable architecture-specific and/or BSP-specific functionality.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

#ifdef GRUB_MULTIBOOT
#include "multiboot.c"
#endif /* GRUB_MULTIBOOT */

void sysHwInit0 (void)
    {
#if defined (INCLUDE_SYMMETRIC_IO_MODE) || defined (INCLUDE_VIRTUAL_WIRE_MODE)
    _func_cpuIndexGet = NULL;
#else
    _func_cpuIndexGet = &dummyGetCpuIndex;
#endif /* (INCLUDE_SYMMETRIC_IO_MODE) || (INCLUDE_VIRTUAL_WIRE_MODE) */

#ifdef  INCLUDE_VXBUS
    sysConfInit0 ();        /* early bootApp dependent initialization */
#endif /* INCLUDE_VXBUS */

#ifdef _WRS_CONFIG_SMP

    /* temporarily use structure to load the brand new GDT into RAM */

    _WRS_WIND_VARS_ARCH_SET(vxCpuIndexGet(),
                            pSysGdt,
                            pSysGdt);
#endif /* _WRS_CONFIG_SMP */

    sysInitGDT ();

#ifdef GRUB_MULTIBOOT
    /* GRUB forces use of DEFAULT_BOOT_LINE */

    *(char*)(BOOT_LINE_ADRS) = 0;

    multibootInit ();
#endif /* GRUB_MULTIBOOT */

    /* initialize CPUID library */

    vxCpuIdLibInit();

    /* perform CPUID probe */

    vxProcessor = sysProcessor = vxCpuIdProbe( &vxCpuId );

    /*
     * for backward compatibility, populate sysCpuId with
     * vxCpuId standard feature structure contents
     */

    bcopy ((char *)&vxCpuId.std, (char *)&sysCpuId, (int)sizeof(VX_CPUID_STD));
    }

#endif  /* INCLUDE_SYS_HW_INIT_0 */

/*******************************************************************************
*
* sysHwInit - initialize the system hardware
*
* This routine initializes various features of the i386/i486 board.
* It is called from usrInit() in usrConfig.c.
*
* NOTE: This routine should not be called directly by the user application.
*
* RETURNS: N/A
*/

void sysHwInit (void)
    {
    PHYS_MEM_DESC *pMmu;
    int ix = 0;

#ifdef _WRS_CONFIG_SMP
    UINT64  cpuKernelVars;
    unsigned int cpuIndex;
#endif /* _WRS_CONFIG_SMP */

#ifdef  INCLUDE_EARLY_I_CACHE_ENABLE
    cacheEnable (INSTRUCTION_CACHE);    /* enable instruction cache */
#endif  /* INCLUDE_EARLY_I_CACHE_ENABLE */
#ifdef  INCLUDE_EARLY_D_CACHE_ENABLE
    cacheEnable (DATA_CACHE);           /* enable data cache */
#endif  /* INCLUDE_EARLY_I_CACHE_ENABLE */

#if defined(INCLUDE_VIRTUAL_WIRE_MODE)
    sysInumTbl = &vwInumTbl[0];
    sysInumTblNumEnt = NELEMENTS (vwInumTbl);
#elif defined(INCLUDE_SYMMETRIC_IO_MODE)
#ifdef _WRS_CONFIG_SMP
    /* store trampoline code space for restoration on warm reboot */

    bcopy ((char *)(CPU_ENTRY_POINT),
                    (char *)(&tramp_code[0]),
                    (int)CPU_AP_AREA);
#endif /* _WRS_CONFIG_SMP */
#else
    sysInumTbl = &dfltInumTbl[0];
    sysInumTblNumEnt = NELEMENTS (dfltInumTbl);
#endif  /* defined(INCLUDE_VIRTUAL_WIRE_MODE) */

#ifdef _WRS_CONFIG_SMP

    /*
     * establish pointers to global descriptor table & task state segment
     * TSS is assigned a cpu unique structure...
     */

    _WRS_WIND_VARS_ARCH_SET(vxCpuIndexGet(),
                            pSysGdt,
                            pSysGdt);

    _WRS_WIND_VARS_ARCH_SET(vxCpuIndexGet(),
                            sysTssCurrent,
                            sysTssCurrent);

    _WRS_WIND_VARS_ARCH_SET(vxCpuIndexGet(),
                            vxIntStackEnabled,
                            1);

    /* establish dedicated segment register for vxKernelVars */

    cpuIndex = vxCpuIndexGet();
    vxKernelVars[cpuIndex].vars.cpu_archVars.cpu_cpuIndex = cpuIndex;
    cpuKernelVars = (UINT64) &vxKernelVars[cpuIndex];
    vxKernelVars[cpuIndex].vars.cpu_archVars.cpu_cpuGsBaseCurrent = cpuKernelVars;
    pentiumMsrSet (IA32_GS_BASE, &cpuKernelVars);

#endif /* _WRS_CONFIG_SMP */

    /* enable the MTRR (Memory Type Range Registers) */

    if ((vxCpuId.std.featuresEdx & CPUID_MTRR) == CPUID_MTRR)
        {
        pentiumMtrrDisable ();      /* disable MTRR */
#   ifdef INCLUDE_MTRR_GET
        (void) pentiumMtrrGet (&sysMtrr); /* get MTRR initialized by BIOS */
#   else
        (void) pentiumMtrrSet (&sysMtrr); /* set your own MTRR */
#   endif /* INCLUDE_MTRR_GET */
        pentiumMtrrEnable ();       /* enable MTRR */
        }

#   ifdef INCLUDE_PMC

    /* enable PMC (Performance Monitoring Counters) */

    pentiumPmcStop ();          /* stop PMC0 and PMC1 */
    pentiumPmcReset ();         /* reset PMC0 and PMC1 */

#   endif /* INCLUDE_PMC */

    /* enable the MCA (Machine Check Architecture) */

    pentiumMcaEnable (TRUE);

#   ifdef INCLUDE_SHOW_ROUTINES

    /*
     * if excMcaInfoShow is not NULL, it is called in the default
     * exception handler when Machine Check Exception happened
     */

    {
    IMPORT FUNCPTR excMcaInfoShow;
    excMcaInfoShow = (FUNCPTR) pentiumMcaShow;
    }
#   endif /* INCLUDE_SHOW_ROUTINES */

#ifdef INCLUDE_SHOW_ROUTINES
    vxShowInit ();
#endif /* INCLUDE_SHOW_ROUTINES */

    /* initialize the number of active mappings (sysPhysMemDescNumEnt) */

    pMmu = &sysPhysMemDesc[0];

    for (ix = 0; ix < NELEMENTS (sysPhysMemDesc); ix++)
        if (pMmu->virtualAddr != (VIRT_ADDR)DUMMY_VIRT_ADDR)
            pMmu++;
        else
            break;

    sysPhysMemDescNumEnt = ix;

#ifdef INCLUDE_BIOS_E820_MEM_AUTOSIZE
    /*
     * Make sure autosize completes prior to vxBus initialization
     */

    sysMemTop();
#endif /* INCLUDE_BIOS_E820_MEM_AUTOSIZE */

    /* load ACPI tables */

    sysConfInit ();        /* bootApp dependent initialization */

    /* initialize PCI library */

#if defined (INCLUDE_USR_MPAPIC) && \
   (defined (INCLUDE_SYMMETRIC_IO_MODE) || defined (INCLUDE_VIRTUAL_WIRE_MODE))

    /* ACPI initialzation writes to static _MP_ struct location */

    usrMpapicInit (TRUE, (char *) MPAPIC_DATA_START);
#endif /* INCLUDE_USR_MPAPIC */

    /* calculate CPU frequency */

    (void)vxCpuIdCalcFreq (&vxCpuId);

#ifdef  INCLUDE_VXBUS

    pSysPlbMethods = pc386PlbMethods;
    _vxb_usDelayRtn = &sysUsDelay;
    hardWareInterFaceInit();

#if defined (INCLUDE_SYMMETRIC_IO_MODE) || defined (INCLUDE_VIRTUAL_WIRE_MODE)
    /* establish link to vxb APIC methods */

    vxbDevIterate(apicIntrIntHelper, NULL, VXB_ITERATE_INSTANCES);
#endif /* INCLUDE_SYMMETRIC_IO_MODE */

    /* Connect intEnable/intDisable function pointers */

    sysIntLvlEnableRtn = (FUNCPTR)sysIntEnablePIC;
    sysIntLvlDisableRtn = (FUNCPTR)sysIntDisablePIC;

    /* initialize the PIC (Programmable Interrupt Controller) */

    sysIntInitPIC ();       /* should be after the PCI init for IOAPIC */
    intEoiGet = sysIntEoiGet;   /* function pointer used in intConnect () */

#if defined (_WRS_CONFIG_SMP)

    /* init IPI vector, assign default handlers... */

    ipiVecInit (INT_NUM_LOAPIC_IPI);

#endif /* (_WRS_CONFIG_SMP) */

#ifdef INCLUDE_PCI_BUS

    /* initialize PCI devices */

    /*
     * PCI-to-PCI bridge initialization should be done here, if it is.
     * It is not necessary for Intel 430HX PCISET, which splits
     * the extended memory area as follows:
     *   - Flash BIOS area from 4GByte to (4GB - 512KB)
     *   - DRAM memory from 1MB to a maximum of 512MB
     *   - PCI memory space from the top of DRAM to (4GB - 512KB)
     */

#if (defined(INCLUDE_SCSI) && defined(INCLUDE_AIC_7880))
    sysAic7880PciInit ();
#endif  /* INCLUDE_SCSI && INCLUDE_AIC_7880 */

#endif /* INCLUDE_PCI_BUS */
#endif /* INCLUDE_VXBUS */

    /* initializes the serial devices */

#ifndef DRV_SIO_NS16550
    sysSerialHwInit ();      /* initialize serial data structure */
#endif /* DRV_SIO_NS16550 */

#if defined(INCLUDE_WINDML) && defined(INCLUDE_PCI_BUS)
#ifndef INCLUDE_VXBUS
    sysWindMLHwInit ();
#endif /*  INCLUDE_VXBUS */
#endif /* INCLUDE_WINDML */
    }

/*******************************************************************************
*
* sysHwInit2 - additional system configuration and initialization
*
* This routine connects system interrupts and does any additional
* configuration necessary.
*
* RETURNS: N/A
*/

void sysHwInit2 (void)
    {

#if defined(INCLUDE_FAST_REBOOT) && defined(INCLUDE_MULTI_STAGE_BOOT) && \
    (!defined(GRUB_MULTIBOOT))

    UINT32 DummyAddress = (UINT32)&sysHwInit2;

    /*
     * Check if we are in the bootrom, works for INCLUDE_BOOT_APP project
     * built bootroms as well. We do this by checking the memory location
     * of the function we are currently executing.
     */

    if (DummyAddress > SYS_RAM_HIGH_ADRS)
        {

        /*
         * save the brand new bootrom image that will be protected by MMU.
         * The last 2 bytes of ROM_SIZE are for the checksum.
         * - compression would minimize the DRAM usage.
         * - when restore, jumping to the saved image would be faster.
         */

        *(UINT32 *)BOOT_IMAGE_ADRS = memRom;

        bcopy ((char *)ROM_TEXT_ADRS, (char *)((ULONG)memRom),
            (*(UINT32 *)BOOT_IMAGE_SIZE));
        *(UINT16 *)((ULONG)memRom + (*(UINT32 *)BOOT_IMAGE_SIZE) - 2) =
            checksum ((UINT16 *)((ULONG)memRom),
                      (*(UINT32 *)BOOT_IMAGE_SIZE) - 2);
        }
#endif  /* !defined(INCLUDE_SYMMETRIC_IO_MODE) && */
        /* defined(INCLUDE_FAST_REBOOT)           */

#ifndef DRV_SIO_NS16550
    /* connect serial interrupt */
    sysSerialHwInit2();
#endif /* DRV_SIO_NS16550 */

    /* connect stray(spurious/phantom) interrupt */

#if defined(INCLUDE_VIRTUAL_WIRE_MODE)
    (void)intConnect (INUM_TO_IVEC (INT_NUM_LOAPIC_SPURIOUS), sysStrayInt, 0);
    (void)intConnect (INUM_TO_IVEC (INT_NUM_GET (LPT_INT_LVL)), sysStrayInt, 0);
#elif defined(INCLUDE_SYMMETRIC_IO_MODE)
    (void)intConnect (INUM_TO_IVEC (INT_NUM_LOAPIC_SPURIOUS), sysStrayInt, 0);
#else
    (void)intConnect (INUM_TO_IVEC (INT_NUM_GET (LPT_INT_LVL)), sysStrayInt, 0);
    (void)intConnect (INUM_TO_IVEC (INT_NUM_GET (PIC_SLAVE_STRAY_INT_LVL)),
                      sysStrayInt, 0);
#endif /* defined(INCLUDE_VIRTUAL_WIRE_MODE) */

#ifndef INCLUDE_VXBUS
#ifdef  INCLUDE_PC_CONSOLE

    /* connect keyboard Controller 8042 chip interrupt */

    (void) intConnect (INUM_TO_IVEC (INT_NUM_GET (KBD_INT_LVL)), kbdIntr, 0);

#endif  /* INCLUDE_PC_CONSOLE */
#endif /* INCLUDE_VXBUS */

#if defined (_WRS_CONFIG_SMP)
    /* connect IPI handlers, up to IPI_MAX_HANDLERS (=8) */

    ipiConnect ((INT_NUM_LOAPIC_IPI + 0), ipiHandlerShutdown);
#endif /* (_WRS_CONFIG_SMP) */

#ifdef INCLUDE_VXBUS

    vxbDevInit();

#ifdef INCLUDE_SIO_UTILS
    sysSerialConnectAll();
#endif /* INCLUDE_SIO_UTILS */

#endif /* INCLUDE_VXBUS */

#ifdef  INCLUDE_DEBUG_STORE
    sysDbgStrInit ();
#endif  /* INCLUDE_DEBUG_STORE */

#ifdef INCLUDE_VXBUS
    taskSpawn("tDevConn", 11, 0, 10000,
              vxbDevConnect, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9);
#endif /* INCLUDE_VXBUS */

    }


/*******************************************************************************
*
* sysPhysMemTop - get the address of the top of physical memory
*
* This routine returns the address of the first missing byte of memory,
* which indicates the top of physical memory.
*
* INTERNAL
* In the case of IA-32 processors, PHYS_MEM_MAX will be 4GB (2^32 bytes) or
* 64GB (2^36 bytes) if the 36-bit Physical Address Extension (PAE) is enabled
* on select processor models.  However, because the tool-chain and sysMemTop()
* API are 32-bit, this routine currently will not auto-size a 36-bit address
* space.  Moreover, this routine will not return the memory top of a platform
* with a memory device using a full 2^32 bytes of address space, as the memory
* top of such a device would be a 33-bit value.
*
* The auto-size code uses the BIOS E820 table to obtain the top of physical
* memory.  Any additional 32-bit addressable memory segments above the
* segment at physical address 0x100000 is mapped using the MMU.  All memory
* locations below physical address 0x100000 are assumed to be reserved
* existing target memory.
*
* RETURNS:  The address of the top of physical memory.
*/

char * sysPhysMemTop (void)
    {
    PHYS_MEM_DESC * pMmu;       /* points to memory desc. table entries */
    UINT32          tempMTP;    /* temporary variable to stop warnings */

    GDTPTR gDtp;
    INT32 nGdtEntries, nGdtBytes;
    unsigned char gdtr[6];      /* temporary GDT register storage */

    BOOL            found = FALSE;

    if (memTopPhys != NULL)
        {
        return (memTopPhys);
        }

#if defined (GRUB_MULTIBOOT) && !defined (INCLUDE_BIOS_E820_MEM_AUTOSIZE)
    {
    ULONG * p = (ULONG *)(MULTIBOOT_SCRATCH);
    MB_INFO* pMbInfo;

    if (*p == MULTIBOOT_BOOTLOADER_MAGIC)
        {
        p = (ULONG *)(MULTIBOOT_SCRATCH + 8);

        pMbInfo = (MB_INFO*)*p;

        memTopPhys = (char *)(((ULONG)pMbInfo->mem_upper << 10) + 0x100000);

        found = TRUE;
        }
    }
#endif /* GRUB_MULTIBOOT  && !INCLUDE_BIOS_E820_MEM_AUTOSIZE */

#ifdef INCLUDE_BIOS_E820_MEM_AUTOSIZE
    {
    char * mem;

    /* get the calculated top of usable memory from BIOS E820 map */

    mem = vxBiosE820MemTopPhys32();
    if( mem != NULL )
        {
        found = TRUE;
        memTopPhys = mem;
        }
    }
#endif /* INCLUDE_BIOS_E820_MEM_AUTOSIZE */

    if (!found)
        {
        memTopPhys = (char *)(LOCAL_MEM_LOCAL_ADRS + LOCAL_MEM_SIZE);
        }

    /* copy the global descriptor table from RAM/ROM to RAM */

    /* Get the Global Data Table Descriptor */

    vxGdtrGet ( (long long int *)&gDtp );

    /* Extract the number of bytes */

    nGdtBytes = (INT32)gDtp.wGdtr[0];

    /* and calculate the number of entries */

    nGdtEntries = (nGdtBytes + 1 ) / sizeof(GDT);

#ifdef _WRS_CONFIG_SMP
    bcopy ((char *)sysGdt,
           (char *)(_WRS_WIND_VARS_ARCH_ACCESS(pSysGdt)),
           nGdtEntries * sizeof(GDT));
#else
    bcopy ((char *)sysGdt, (char *)pSysGdt, nGdtEntries * sizeof(GDT));
#endif /* _WRS_CONFIG_SMP */

    /*
     * We assume that there are no memory mapped IO addresses
     * above the "memTopPhys" if INCLUDE_PCI_BUS is not defined.
     * Thus we set the "limit" to get the General Protection Fault
     * when the memory above the "memTopPhys" is accessed.
     */

#if     !defined (INCLUDE_PCI_BUS) && \
    !defined (INCLUDE_MMU_BASIC) && !defined (INCLUDE_MMU_FULL)
    {
    int   ix;
    int   limit = (((UINT32) memTopPhys) >> 12) - 1;
#ifdef _WRS_CONFIG_SMP
    GDT *pGdt = (_WRS_WIND_VARS_ARCH_ACCESS(pSysGdt));
#else
    GDT * pGdt  = pSysGdt;
#endif /* _WRS_CONFIG_SMP */

    for (ix = 1; ix < GDT_ENTRIES; ++ix)
        {
        ++pGdt;
        pGdt->limit00 = limit & 0x0ffff;
        pGdt->limit01 = ((limit & 0xf0000) >> 16) | (pGdt->limit01 & 0xf0);
        }
    }
#endif  /* INCLUDE_PCI_BUS */

    /* load the global descriptor table. set the MMU table */

    *(short *) &gdtr[0] = GDT_ENTRIES * sizeof (GDT) - 1;
#ifdef _WRS_CONFIG_SMP
    *(int *) &gdtr[2] = (int)(_WRS_WIND_VARS_ARCH_ACCESS(pSysGdt));
#else
    *(int *) &gdtr[2] = (int)(pSysGdt);
#endif /* _WRS_CONFIG_SMP */

    sysLoadGdt ((char *)gdtr);

    /* set the MMU descriptor table */

    tempMTP    =
        (UINT32)memTopPhys & ~(VM_PAGE_SIZE - 1); /* VM_PAGE_SIZE aligned */
    memTopPhys = (char *)tempMTP;

#if (VM_PAGE_SIZE == PAGE_SIZE_4KB)
    pMmu = &sysPhysMemDesc[4];      /* 5th entry: above 1.5MB upper memory */
    pMmu->len = (UINT32) memTopPhys - (UINT32) pMmu->physicalAddr;
#else   /* (VM_PAGE_SIZE == PAGE_SIZE_4KB) */
    pMmu = &sysPhysMemDesc[2];      /* 3rd entry: above 8MB upper memory */
    pMmu->len = (UINT32) memTopPhys - (UINT32) pMmu->physicalAddr;
#endif  /* (VM_PAGE_SIZE == PAGE_SIZE_4KB) */

#if defined INCLUDE_BIOS_E820_MEM_AUTOSIZE
    {
    BIOS_E820_MAP_DESC mapDesc;
    unsigned int ix,adr32;

    /* map any additional segments above the LOCAL_MEM_LOCAL_ADRS segment */

    for (ix = 0; ; ix++)
        {

        /*
         * for each index continue until vxBiosE820MapDescGet() returns ERROR
         */

        if (vxBiosE820MapDescGet (ix, &mapDesc) == ERROR)
            break;

        /*
         * only map usable entries
         * do not map segment at address 0x0
         * segment at LOCAL_MEM_LOCAL_ADRS already mapped
         * must be a 32 bit addressable segment
         * must bigger than 32-bit atom max page size
         */

        if (( mapDesc.type == BIOS_E820_MEM_TYPE_USABLE) &&
            ( mapDesc.addr != 0x0 ) &&
            ( mapDesc.addr != LOCAL_MEM_LOCAL_ADRS) &&
            ( mapDesc.addr < SIZE_ADDR_SPACE_32 ) &&
            ( mapDesc.len >= PAGE_SIZE_4MB ))
            {
            adr32 = mapDesc.addr;
            if (sysMmuMapAdd ((void *)adr32,mapDesc.len,
                              VM_STATE_MASK_FOR_ALL,
                              VM_STATE_FOR_MEM_APPLICATION) == ERROR)
                break;
            }
        }
    }
#endif /* INCLUDE_BIOS_E820_MEM_AUTOSIZE */

    return (memTopPhys);
    }

/*******************************************************************************
*
* sysMemTop - get the address of the top of VxWorks memory
*
* This routine returns a pointer to the first byte of memory not
* controlled or used by VxWorks.
*
* The user can reserve memory space by defining the macro USER_RESERVED_MEM
* in config.h.  This routine returns the address of the reserved memory
* area.  The value of USER_RESERVED_MEM is in bytes.
*
* RETURNS: The address of the top of VxWorks memory.
*/

char * sysMemTop (void)
    {
    static char * memTop = NULL;

    if (memTop == NULL)
        {
        memTop = sysPhysMemTop ();

#if defined(INCLUDE_FAST_REBOOT) && defined(INCLUDE_MULTI_STAGE_BOOT) && \
    (!defined(GRUB_MULTIBOOT))

        /*
         * Subtract the size of the boot image from the top of memory and
         * record it here via memTop, so that we are globally aware how much
         * are left.
         */

        memRomSize = ROUND_UP((*(UINT32 *)BOOT_IMAGE_SIZE), VM_PAGE_SIZE);
        memTop = memTop - memRomSize;
        memRom = (UINT32) ROUND_DOWN(((UINT32)memTop), VM_PAGE_SIZE);
#endif  /* !defined(INCLUDE_SYMMETRIC_IO_MODE) && */
        /* defined(INCLUDE_FAST_REBOOT)           */

        /* Account for user reserved memory */

        memTop -= USER_RESERVED_MEM;

#ifdef INCLUDE_EDR_PM

        /* account for ED&R persistent memory */

        memTop = memTop - PM_RESERVED_MEM;
#endif /* INCLUDE_EDR_PM */

        if ((UINT32)(&end) < 0x100000)      /* this is for bootrom */
            memTop = (char *)EBDA_START;    /* preserve the MP table */
        else if ((UINT32)(&end) < SYS_RAM_LOW_ADRS) /* bootrom in upper mem */
            memTop = (char *)(SYS_RAM_LOW_ADRS & 0xfff00000);
        }

    return (memTop);
    }

/*******************************************************************************
*
* sysToMonitor - transfer control to the ROM monitor
*
* This routine transfers control to the ROM monitor.  It is usually called
* only by reboot() -- which services ^X -- and by bus errors at interrupt
* level.  However, in some circumstances, the user may wish to introduce a
* new <startType> to enable special boot ROM facilities.
*
* RETURNS: Does not return.
*/

STATUS sysToMonitor
    (
    int startType   /* passed to ROM to tell it how to boot */
    )
    {
    FUNCPTR pEntry;
    INT16 * pDst;
#if defined (INCLUDE_SYMMETRIC_IO_MODE) || defined (INCLUDE_VIRTUAL_WIRE_MODE)
    BOOL enable = FALSE;
#endif /* defined (INCLUDE_SYMMETRIC_IO_MODE) || */
       /* defined (INCLUDE_VIRTUAL_WIRE_MODE)    */

    /* ADLINK官方提供的bsp ctrl+x复位存在问题，为了调试方便更改为ctrl+x就进行硬重启 wming 时间：2016年10月14日 11:28:20 */
    goto sysToMonitorCold;

#if defined(_WRS_CONFIG_SMP)
    int cPu = vxCpuPhysIndexGet();
#endif /* defined(_WRS_CONFIG_SMP) */

#if ((!defined(INCLUDE_FAST_REBOOT)) &&                 \
     (!defined(INCLUDE_MULTI_STAGE_WARM_REBOOT)) &&     \
     defined(INCLUDE_MULTI_STAGE_BOOT))

    /* Do cold reboots for non-fast-reboot multi-stage boot. */

    startType = BOOT_COLD;
#endif /* GRUB_MULTIBOOT */
    
    /* 
     * check if boot from GRUB, The GRUB vxWorks image does 
     * not support "warm" reboot.
     */
    
    if (*((UINT32 *)(MULTIBOOT_SCRATCH)) ==
        MULTIBOOT_BOOTLOADER_MAGIC)
        {
        startType = BOOT_COLD;
        }

    sysConfStop ();  /* shutdown BOOTAPP dependent devices */

    intCpuLock();        /* LOCK INTERRUPTS */

#ifdef _WRS_CONFIG_SMP
    /*
     * At this point we are either the bootstrap processor,
     * or an application processor...
     *
     * ...need to reset APs and go out on the bootstrap processor...
     */

    if (vxCpuIndexGet() != 0)
        {
        /* CPCs use ipiId = 0 */

        if (vxIpiConnect (0, (IPI_HANDLER_FUNC) (sysToMonitor),
                         (void *) startType) == ERROR)
            goto sysToMonitorCold;

        if (vxIpiEmit(0, 1) == ERROR)
            goto sysToMonitorCold;

        taskCpuLock();       /* if not in interrupt context, taskCpuLock */

        ipiShutdownSup();

        }

    sysCpuStop_APs ();   /* Stop APs*/
    sysUsDelay(50000);

    sysCpuReset_APs ();   /* place APs in reset */
    sysDelay();
#endif /* _WRS_CONFIG_SMP */

    /* now running on BSP, APs in reset... */

#if defined (INCLUDE_MMU_P6_32BIT)
    mmuPro32Enable (FALSE);          /* disable MMU */
#else
    mmuPro36Enable (FALSE);          /* disable MMU */
#endif /* defined (INCLUDE_MMU_P6_32BIT) */

#if defined(INCLUDE_SYMMETRIC_IO_MODE)
#ifdef _WRS_CONFIG_SMP
    /* restore trampoline code space for warm reboot */

    if ( cPu == 0 )
    bcopy ((char *)(&tramp_code[0]),
           (char *)(CPU_ENTRY_POINT),
           (int)CPU_AP_AREA);
#endif /* _WRS_CONFIG_SMP */
#endif  /* defined(INCLUDE_SYMMETRIC_IO_MODE) */

    /* If this is a cold reboot */

    if (startType == BOOT_COLD)
        goto sysToMonitorCold;

    pDst = (short *)SYS_RAM_HIGH_ADRS;  /* copy it in lower mem */
    pEntry = (FUNCPTR)(ROM_TEXT_ADRS + ROM_WARM_HIGH);

#if defined(INCLUDE_MULTI_STAGE_BOOT) && \
    (!(defined(INCLUDE_BOOT_APP))) && \
    (!(defined(INCLUDE_BOOT_RAM_IMAGE)))
    pEntry = (FUNCPTR)(FIRST_TEXT_ADRS + ROM_WARM_HIGH);
#else /* INCLUDE_MULTI_STAGE_BOOT */
    pEntry = (FUNCPTR)(ROM_TEXT_ADRS + ROM_WARM_HIGH);
#endif /* INCLUDE_MULTI_STAGE_BOOT */

#if defined(INCLUDE_FAST_REBOOT) && defined(INCLUDE_MULTI_STAGE_BOOT) && \
    (!defined(INCLUDE_BOOT_APP)) && (!defined(GRUB_MULTIBOOT))

    /*
     * Make sure our stored checksum is still valid, else do a cold boot
     */

    if (*(UINT16 *)((ULONG)memRom + (*(UINT32 *)BOOT_IMAGE_SIZE) - 2) !=
        checksum ((UINT16 *)((ULONG)memRom), (*(UINT32 *)BOOT_IMAGE_SIZE) - 2))
        {
        *(UINT32 *)BOOT_IMAGE_ADRS = 0;
        *(UINT32 *)BOOT_IMAGE_SIZE = 0;
        goto sysToMonitorCold;
        }

    /*
     * If either the memRomSize is invalid or the location where it was
     * copied, we know it's not valid - do a cold reboot instead.
     */

    if (!((memRom > 0) && (memRomSize > 0)))
        goto sysToMonitorCold;

    /*
     * vxStage1Boot.s uses the stored values in BOOT_IMAGE_ADRS and
     * BOOT_IMAGE_SIZE in the function reBoot, to determine if the
     * bootrom image was copied and can be restored, before doing a
     * restore and then a jump to warm boot.
     * In sysLib.c, function sysPhysMemTop, we fill in BOOT_IMAGE_ADRS
     * when the copy takes  place, and the BOOT_IMAGE_SIZE gets filled
     * in by vxStage1Boot.s function bootProtected2, just before jumping
     * to stage 2.
     */

    goto sysToMonitorWarm;

#endif  /* defined(INCLUDE_FAST_REBOOT) && defined(INCLUDE_MULTI_STAGE_BOOT) */

    /* If here because of fatal error during an interrupt just reboot */

#ifdef _WRS_CONFIG_SMP
    if(INT_CONTEXT())
#else
    if(intCnt > 0)
#endif /* _WRS_CONFIG_SMP */
        {
        pEntry = (FUNCPTR)(ROM_TEXT_ADRS + ROM_WARM_HIGH);

        sysClkDisable ();   /* disable the system clock interrupt */
        sysIntLock ();      /* lock the used/owned interrupts */

#if defined (INCLUDE_SYMMETRIC_IO_MODE) || defined (INCLUDE_VIRTUAL_WIRE_MODE)
            /* Shutdown Local APIC */

            (*getLoApicIntrEnable)(loApicDevID, (void *) &enable);
#endif /* defined (INCLUDE_SYMMETRIC_IO_MODE) || */
       /* defined (INCLUDE_VIRTUAL_WIRE_MODE)    */

#if defined (INCLUDE_SYMMETRIC_IO_MODE)
            /* Restore Local APIC initial settings prior to enable */

            *(volatile int *)(LOAPIC_BASE + 0x0f0) = /* LOAPIC_SVR */
                (int)glbLoApicOldSvr;
            *(volatile int *)(LOAPIC_BASE + 0x350) = /* LOAPIC_LINT0 */
                (int)glbLoApicOldLint0;
            *(volatile int *)(LOAPIC_BASE + 0x360) = /* LOAPIC_LINT1 */
                (int)glbLoApicOldLint1;

#endif /*  defined (INCLUDE_SYMMETRIC_IO_MODE) */

            (*pEntry) (startType);

        /* Oops, This should not happen - Reset */

        goto sysToMonitorCold;
        }

    /* jump to the warm start entry point */

#if defined(INCLUDE_FAST_REBOOT) && defined(INCLUDE_MULTI_STAGE_BOOT) && \
        (!defined(INCLUDE_BOOT_APP)) && (!defined(GRUB_MULTIBOOT))
sysToMonitorWarm:       /* cleanup and jump to the entry point */
#endif /* defined(INCLUDE_FAST_REBOOT) && defined(INCLUDE_MULTI_STAGE_BOOT) */

    sysClkDisable ();   /* disable the system clock interrupt */
    sysIntLock ();      /* lock the used/owned interrupts */

#if defined (INCLUDE_SYMMETRIC_IO_MODE) || defined (INCLUDE_VIRTUAL_WIRE_MODE)
    /* Shutdown Local APIC */

    (*getLoApicIntrEnable)(loApicDevID, (void *) &enable);
#endif /* defined (INCLUDE_SYMMETRIC_IO_MODE) || */
       /* defined (INCLUDE_VIRTUAL_WIRE_MODE)    */

#if defined (INCLUDE_SYMMETRIC_IO_MODE)
    /* Restore Local APIC initial settings prior to enable */

    *(volatile int *)(LOAPIC_BASE + 0x0f0) = /* LOAPIC_SVR */
    (int)glbLoApicOldSvr;
    *(volatile int *)(LOAPIC_BASE + 0x350) = /* LOAPIC_LINT0 */
    (int)glbLoApicOldLint0;
    *(volatile int *)(LOAPIC_BASE + 0x360) = /* LOAPIC_LINT1 */
    (int)glbLoApicOldLint1;
#endif /*  defined (INCLUDE_SYMMETRIC_IO_MODE) */

    (*pEntry) (startType);

sysToMonitorCold:

    /* perform the cold boot since the warm boot is not possible */

    sysClkDisable ();

    /* Reset Control Register, Reset CPU and System Reset Register */

    sysHardReset ();

    return (OK);    /* in case we ever continue from ROM monitor */
    }

/*******************************************************************************
*
* sysIntInitPIC - initialize the interrupt controller
*
* This routine initializes the interrupt controller.
* Maps APIC Memory space.
*
* RETURNS: N/A
*
*/

void sysIntInitPIC (void)
    {
#if defined(INCLUDE_VIRTUAL_WIRE_MODE)
    {
    /* Map Local APIC Address space... */

    UINT32 mpApicloBase;
    UINT32 lengthLo;    /* length of Local APIC registers */

    i8259Init ();

    /* add an entry to the sysMmuPhysDesc[] for Local APIC */

    (*getMpApicloBaseGet) (mpApicDevID, &mpApicloBase);
    lengthLo = ((UINT32)LOAPIC_LENGTH / VM_PAGE_SIZE) * VM_PAGE_SIZE;

    sysMmuMapAdd ((void *)mpApicloBase, lengthLo,
                  VM_STATE_MASK_FOR_ALL, VM_STATE_FOR_IO);
    }
#elif   defined(INCLUDE_SYMMETRIC_IO_MODE)
    {
    /* Map Local and IO APIC Address spaces... */

    UINT32 lengthLo;    /* length of Local APIC registers */
    UINT32 lengthIo;    /* length of IO APIC registers */

    int    i;

    UINT32 mpApicloBase;
    UINT32 mpApicNioApic;

    UINT32 *mpApicAddrTable;

    /* add an entry to the sysMmuPhysDesc[] for Local APIC and IO APIC */
    /* only do this once...                                            */

    (*getMpApicloBaseGet) (mpApicDevID, &mpApicloBase);
    lengthLo = ((UINT32)LOAPIC_LENGTH / VM_PAGE_SIZE) * VM_PAGE_SIZE;

    sysMmuMapAdd ((void *)mpApicloBase, lengthLo,
                  VM_STATE_MASK_FOR_ALL, VM_STATE_FOR_IO);

    (*getMpApicNioApicGet) (mpApicDevID, &mpApicNioApic);
    (*getMpApicAddrTableGet)(mpApicDevID, (void *)&mpApicAddrTable);

    lengthIo = ((UINT32)IOAPIC_LENGTH / VM_PAGE_SIZE) * VM_PAGE_SIZE;

    for(i=0; i<mpApicNioApic; i++)
        {
        sysMmuMapAdd ((void *)(mpApicAddrTable[i]), lengthIo,
                      VM_STATE_MASK_FOR_ALL, VM_STATE_FOR_IO);
        }
    }
#else
    i8259Init ();
#endif  /* defined(INCLUDE_VIRTUAL_WIRE_MODE) */
    }

#if defined (INCLUDE_SYMMETRIC_IO_MODE) || defined (INCLUDE_VIRTUAL_WIRE_MODE)
/*******************************************************************************
*
* apicIntrIntHelper - initialize pointers to vxBus driver routines.
*
* This routine initializes pointers to vxBus driver routines up front, rather
* than calling into vxBus API...
*
* Called from "sysPciPirqEnable" because we require intEnable/Disable early on
* during PCI initialization...
*
* RETURNS: STATUS
*
*/

STATUS apicIntrIntHelper
    (
    VXB_DEVICE_ID pDev,
    void * unused
    )
    {
    FUNCPTR func;

    /* check for MP APIC */

    func = vxbDevMethodGet(pDev, DEVMETHOD_CALL(mpApicLoIndexTableGet));
    if ( func != NULL )
        {
        mpApicDevID = pDev;
        getMpApicLoIndexTable = func;
        }

    func = vxbDevMethodGet(pDev, DEVMETHOD_CALL(mpApicloBaseGet));
    if ( func != NULL )
        {
        mpApicDevID = pDev;
        getMpApicloBaseGet = func;
        }

#if defined(INCLUDE_SYMMETRIC_IO_MODE)

    /* check for IO APIC */

    func = vxbDevMethodGet(pDev, DEVMETHOD_CALL(mpApicNioApicGet));
    if ( func != NULL )
        {
        mpApicDevID = pDev;
        getMpApicNioApicGet = func;
        }

    func = vxbDevMethodGet(pDev, DEVMETHOD_CALL(mpApicAddrTableGet));
    if ( func != NULL )
        {
        mpApicDevID = pDev;
        getMpApicAddrTableGet = func;
        }

    func = vxbDevMethodGet(pDev, DEVMETHOD_CALL(ioApicRedNumEntriesGet));
    if ( func != NULL )
        {
        ioApicDevID = pDev;
        getIoApicRedNumEntriesGet = func;
        }

    getIoApicIntrIntEnable = NULL;
    func = vxbDevMethodGet(pDev, DEVMETHOD_CALL(ioApicIntrIntEnable));
    if ( func != NULL )
        {
        ioApicDevID = pDev;
        getIoApicIntrIntEnable = func;
        }

    getIoApicIntrIntDisable = NULL;
    func = vxbDevMethodGet(pDev, DEVMETHOD_CALL(ioApicIntrIntDisable));
    if ( func != NULL )
        {
        ioApicDevID = pDev;
        getIoApicIntrIntDisable = func;
        }

    func = vxbDevMethodGet(pDev, DEVMETHOD_CALL(ioApicIntrIntLock));
    if ( func != NULL )
        {
        ioApicDevID = pDev;
        getIoApicIntrIntLock = func;
        }

    func = vxbDevMethodGet(pDev, DEVMETHOD_CALL(ioApicIntrIntUnlock));
    if ( func != NULL )
        {
        ioApicDevID = pDev;
        getIoApicIntrIntUnlock = func;
        }
#endif /* defined(INCLUDE_SYMMETRIC_IO_MODE) */

    /* check for Local APIC */

    func = vxbDevMethodGet(pDev, DEVMETHOD_CALL(loApicIntrInitAP));
    if ( func != NULL )
        {
        loApicDevID = pDev;
        getLoApicIntrInitAP = func;
        }

    func = vxbDevMethodGet(pDev, DEVMETHOD_CALL(loApicIntrEnable));
    if ( func != NULL )
        {
        loApicDevID = pDev;
        getLoApicIntrEnable = func;
        }

    func = vxbDevMethodGet(pDev, DEVMETHOD_CALL(loApicIntrIntLock));
    if ( func != NULL )
        {
        loApicDevID = pDev;
        getLoApicIntrIntLock = func;
        }

    func = vxbDevMethodGet(pDev, DEVMETHOD_CALL(loApicIntrIntUnlock));
    if ( func != NULL )
        {
        loApicDevID = pDev;
        getLoApicIntrIntUnlock = func;
        }

    return(OK);
    }
#else

/*******************************************************************************
*
* dummyGetCpuIndex - dummy up the UP implementation of vxCpuIndexGet
*
* Arch or BSP system-dependent implementation routine,
* used to dummy up the UP implementation of vxCpuIndexGet.
* Under UP always 0.
*
* RETURNS: 0
*
* \NOMANUAL
*/

unsigned int dummyGetCpuIndex (void)
    {
    return (0);
    }
#endif /* defined (INCLUDE_SYMMETRIC_IO_MODE) || */
       /* defined (INCLUDE_VIRTUAL_WIRE_MODE)    */

/*******************************************************************************
*
* sysIntLock - lock out all interrupts
*
* This routine saves the mask and locks out all interrupts.
*
* SEE ALSO: sysIntUnlock()
*
*/

VOID sysIntLock (void)
    {
    INT32 oldLevel = intCpuLock();  /* LOCK INTERRUPTS */

#if defined (INCLUDE_SYMMETRIC_IO_MODE) || defined (INCLUDE_VIRTUAL_WIRE_MODE)
    int dummy = 0xdead;
#endif /* INCLUDE_SYMMETRIC_IO_MODE || INCLUDE_VIRTUAL_WIRE_MODE */

#if defined(INCLUDE_VIRTUAL_WIRE_MODE)
    (*getLoApicIntrIntLock)(loApicDevID, (void *) &dummy);
    i8259IntLock ();
#elif defined(INCLUDE_SYMMETRIC_IO_MODE)
    (*getLoApicIntrIntLock)(loApicDevID, (void *) &dummy);
#else
    i8259IntLock ();
#endif  /* defined(INCLUDE_VIRTUAL_WIRE_MODE) */
    intCpuUnlock(oldLevel);         /* UNLOCK INTERRUPTS */
    }

/*******************************************************************************
*
* sysIntUnlock - unlock the PIC interrupts
*
* This routine restores the mask and unlocks the PIC interrupts
*
* SEE ALSO: sysIntLock()
*
*/

VOID sysIntUnlock (void)
    {
    INT32 oldLevel = intCpuLock();  /* LOCK INTERRUPTS */

#if defined (INCLUDE_SYMMETRIC_IO_MODE) || defined (INCLUDE_VIRTUAL_WIRE_MODE)
    int dummy = 0xdead;
#endif /* INCLUDE_SYMMETRIC_IO_MODE || INCLUDE_VIRTUAL_WIRE_MODE */

#if defined(INCLUDE_VIRTUAL_WIRE_MODE)
    (*getLoApicIntrIntUnlock)(loApicDevID, (void *) &dummy);
    i8259IntUnlock ();
#elif defined(INCLUDE_SYMMETRIC_IO_MODE)
    (*getLoApicIntrIntUnlock)(loApicDevID, (void *) &dummy);
#else
    i8259IntUnlock ();
#endif /* defined(INCLUDE_VIRTUAL_WIRE_MODE) */
    intCpuUnlock(oldLevel);         /* UNLOCK INTERRUPTS */
    }

/*******************************************************************************
*
* sysIntDisablePIC - disable a bus interrupt level
*
* This routine disables a specified bus interrupt level.
*
* RETURNS: OK, or ERROR if failed.
*
*/

STATUS sysIntDisablePIC
    (
    int irqNo       /* IRQ(PIC) or INTIN(APIC) number to disable */
    )
    {
#if defined(INCLUDE_SYMMETRIC_IO_MODE)
    if (getIoApicIntrIntDisable != NULL)
        return ((*getIoApicIntrIntDisable)(ioApicDevID, (void *)&irqNo));
    else
        return (vxbDevMethodRun(DEVMETHOD_CALL(ioApicIntrIntDisable),
                                (void *)&irqNo));
#else
    return (i8259IntDisable (irqNo));
#endif /* defined(INCLUDE_VIRTUAL_WIRE_MODE) */
    }

/*******************************************************************************
*
* sysIntEnablePIC - enable a bus interrupt level
*
* This routine enables a specified bus interrupt level.
*
* RETURNS: OK, or ERROR if failed.
*
*/

STATUS sysIntEnablePIC
    (
    int irqNo       /* IRQ(PIC) or INTIN(APIC) number to enable */
    )
    {
#if defined(INCLUDE_SYMMETRIC_IO_MODE)
    if (getIoApicIntrIntEnable != NULL)
        return ((*getIoApicIntrIntEnable)(ioApicDevID, (void *)&irqNo));
    else
        return (vxbDevMethodRun(DEVMETHOD_CALL(ioApicIntrIntEnable),
                                (void *)&irqNo));
#else
    return (i8259IntEnable (irqNo));
#endif /* defined(INCLUDE_VIRTUAL_WIRE_MODE) */
    }

#ifdef _WRS_VX_IA_IPI_INSTRUMENTATION
extern struct ipiCounters *ipiCounters[VX_MAX_SMP_CPUS];
#endif /* _WRS_VX_IA_IPI_INSTRUMENTATION */

#if defined (INCLUDE_SYMMETRIC_IO_MODE) || defined (INCLUDE_VIRTUAL_WIRE_MODE)
/*******************************************************************************
* sysLoApicIntEoi -  send EOI (End Of Interrupt) signal to Local APIC
*
* This routine sends an EOI signal to the Local APIC's interrupting source.
*
* RETURNS: N/A
*/

void sysLoApicIntEoi
    (
    INT32 irqNo         /* INIIN number to send EOI */
    )
    {
    *(int *)(LOAPIC_BASE + LOAPIC_EOI) = 0;
    }

#ifdef _WRS_VX_IA_IPI_INSTRUMENTATION

/*******************************************************************************
* sysLoApicIpiBoi -  Called before the interrupt service routine to increment
*                    IPI instrumentation interrupt count
*
* This routine is called before the interrupt service routine to increment
* IPI instrumentation interrupt count.
*
* RETURNS: N/A
*/

void sysLoApicIpiBoi
    (
    INT32 irqNo
    )
    {
    int ipiNum =  irqNo - INT_NUM_LOAPIC_IPI;
    struct ipiCounters *ipiCnt = ipiCounters[vxCpuPhysIndexGet()];

    if (ipiNum >= 0 && ipiNum < 7)
        vxAtomicInc((atomic_t *)(&(ipiCnt->ipiRecv[ipiNum])));
    }
#endif /* _WRS_VX_IA_IPI_INSTRUMENTATION */

#endif /* INCLUDE_SYMMETRIC_IO_MODE || INCLUDE_VIRTUAL_WIRE_MODE */

/*******************************************************************************
*
* sysIntEoiGet - get EOI/BOI function and its parameter
*
* This routine gets EOI function and its parameter for the interrupt controller.
* If returned EOI/BOI function is NULL, intHandlerCreateI86() replaces
* "call _routineBoi/Eoi" in intConnectCode[] with NOP instruction.
*
* RETURNS: N/A
*
*/

void sysIntEoiGet
    (
    VOIDFUNCPTR * vector,   /* interrupt vector to attach to */
    VOIDFUNCPTR * routineBoi,   /* BOI function */
    int * parameterBoi,     /* a parameter of the BOI function */
    VOIDFUNCPTR * routineEoi,   /* EOI function */
    int * parameterEoi      /* a parameter of the EOI function */
    )
    {
    int intNum = IVEC_TO_INUM (vector);
    int irqNo;

    /* set default BOI routine & parameter */

    *routineBoi   = NULL;
    *parameterBoi = 0;

    /* find a match in sysInumTbl[] */

    for (irqNo = 0; irqNo < sysInumTblNumEnt; irqNo++)
        {
        if (sysInumTbl[irqNo] == intNum)
            break;
        }

    *parameterEoi = irqNo;  /* irq is sysInumTblNumEnt, if no match */

#if defined (INCLUDE_SYMMETRIC_IO_MODE)

    if (*parameterEoi == sysInumTblNumEnt)
        *parameterEoi = intNum;  /* irq is requested intNum, if no match */

    if (intNum == INT_NUM_LOAPIC_SPURIOUS)
        *routineEoi = NULL;        /* no EOI is necessary */
    else
        *routineEoi = sysLoApicIntEoi; /* set Local APIC's EOI routine */

#ifdef _WRS_VX_IA_IPI_INSTRUMENTATION
    /*
     * Add Ipi Counter BOI
     */

    if (intNum >= INT_NUM_LOAPIC_IPI &&
        intNum < (INT_NUM_LOAPIC_IPI + IPI_MAX_HANDLERS))
        {
        *routineBoi = sysLoApicIpiBoi;
        *parameterBoi = intNum;
        }
#endif /* _WRS_VX_IA_IPI_INSTRUMENTATION */

#else

#if defined (INCLUDE_VIRTUAL_WIRE_MODE)

    if (irqNo >= N_PIC_IRQS)        /* IRQ belongs to the Local APIC */
        {
        if (intNum == INT_NUM_LOAPIC_SPURIOUS)
            *routineEoi = NULL;     /* no EOI is necessary */
        else
            *routineEoi = sysLoApicIntEoi; /* set Local APIC's EOI routine */
        return;
        }

#endif /* INCLUDE_VIRTUAL_WIRE_MODE */

    /* set the [BE]OI parameter for the master & slave PIC */

    *parameterBoi = irqNo;
    *parameterEoi = irqNo;

    /* set the right BOI routine */

    if (irqNo == 0)         /* IRQ0 BOI routine */
        {
#if (PIC_IRQ0_MODE == PIC_AUTO_EOI)
        *routineBoi   = NULL;
#elif   (PIC_IRQ0_MODE == PIC_EARLY_EOI_IRQ0)
        *routineBoi   = i8259IntBoiEem;
#elif   (PIC_IRQ0_MODE == PIC_SPECIAL_MASK_MODE_IRQ0)
        *routineBoi   = i8259IntBoiSmm;
#else
        *routineBoi   = NULL;
#endif  /* (PIC_IRQ0_MODE == PIC_AUTO_EOI) */
        }
#if !defined (INCLUDE_VIRTUAL_WIRE_MODE)
    else if ((irqNo == PIC_MASTER_STRAY_INT_LVL) ||
             (irqNo == PIC_SLAVE_STRAY_INT_LVL))
        {
        *routineBoi   = i8259IntBoi;
        }
#endif /* INCLUDE_VIRTUAL_WIRE_MODE */

    /* set the right EOI routine */

    if (irqNo == 0)         /* IRQ0 EOI routine */
        {
#if (PIC_IRQ0_MODE == PIC_AUTO_EOI) || \
    (PIC_IRQ0_MODE == PIC_EARLY_EOI_IRQ0)
        *routineEoi   = NULL;
#elif   (PIC_IRQ0_MODE == PIC_SPECIAL_MASK_MODE_IRQ0)
        *routineEoi   = i8259IntEoiSmm;
#else
        *routineEoi   = i8259IntEoiMaster;
#endif  /* (PIC_IRQ0_MODE == PIC_AUTO_EOI) || (PIC_EARLY_EOI_IRQ0) */
        }
    else if (irqNo < 8)         /* IRQ[1-7] EOI routine */
        {
#if (PIC_IRQ0_MODE == PIC_AUTO_EOI)
        *routineEoi   = NULL;
#else
        *routineEoi   = i8259IntEoiMaster;
#endif  /* (PIC_IRQ0_MODE == PIC_AUTO_EOI) */
        }
    else                /* IRQ[8-15] EOI routine */
        {
#if defined (PIC_SPECIAL_FULLY_NESTED_MODE)
        *routineEoi   = i8259IntEoiSlaveSfnm;
#else
        *routineEoi   = i8259IntEoiSlaveNfnm;
#endif  /* defined (PIC_SPECIAL_FULLY_NESTED_MODE) */
        }
#endif  /* INCLUDE_SYMMETRIC_IO_MODE */
    }

/*******************************************************************************
*
* sysIntLevel - get an IRQ(PIC) or INTIN(APIC) number in service
*
* This routine gets an IRQ(PIC) or INTIN(APIC) number in service.
* We assume the following:
*   - this function is called in intEnt()
*   - IRQ number of the interrupt is at intConnectCode [29]
*
* RETURNS: 0 - (sysInumTblNumEnt - 1), or sysInumTblNumEnt if we failed to get
*          it.
*
*/

int sysIntLevel
    (
    int arg     /* parameter to get the stack pointer */
    )
    {
    UINT32 * pStack;
    UCHAR * pInst;
    INT32 ix;
    INT32 irqNo = sysInumTblNumEnt; /* return sysInumTblNumEnt if we failed */

    pStack = (UINT32 *)&arg;    /* get the stack pointer */
    pStack += 3;        /* skip pushed volatile registers */

    /*
     * we are looking for a return address on the stack which point
     * to the next instruction of "call _intEnt" in the malloced stub.
     * Then get the irqNo at intConnectCode [29].
     */

    for (ix = 0; ix < 10; ix++, pStack++)
        {
        pInst = (UCHAR *)*pStack;       /* return address */
        if ((*pInst == 0x50) &&         /* intConnectCode [5] */
            ((*(int *)(pInst - 4) + (int)pInst) == (int)intEnt))
            {
            irqNo = *(int *)(pInst + 24);   /* intConnectCode [29] */
            break;
            }
        }

    return (irqNo);
    }

/****************************************************************************
*
* sysProcNumGet - get the processor number
*
* This routine returns the processor number for the CPU board, which is
* set with sysProcNumSet().
*
* RETURNS: The processor number for the CPU board.
*
* SEE ALSO: sysProcNumSet()
*/

int sysProcNumGet (void)
    {
    return (sysProcNum);
    }

/****************************************************************************
*
* sysProcNumSet - set the processor number
*
* Set the processor number for the CPU board.  Processor numbers should be
* unique on a single backplane.
*
* NOTE: By convention, only Processor 0 should dual-port its memory.
*
* RETURNS: N/A
*
* SEE ALSO: sysProcNumGet()
*/

void sysProcNumSet
    (
    int procNum     /* processor number */
    )
    {
    sysProcNum = procNum;
    }

/*******************************************************************************
*
* sysDelay - allow recovery time for port accesses
*
* This routine provides a brief delay used between accesses to the same serial
* port chip.
*
* RETURNS: N/A
*/

void sysDelay (void)
    {
    char ix;

    ix = sysInByte (UNUSED_ISA_IO_ADDRESS); /* it takes 720ns */
    }

/*******************************************************************************
*
* sysUsDelay - delay specified number of microseconds
*
* This routine will delay specified number of microseconds by calling 
* pentiumTSCDelay().
*
* RETURNS: N/A
*/

void sysUsDelay
    (
    int uSec
    )
    {
    pentiumTSCDelay(uSec);
    }

/*******************************************************************************
*
* sysStrayInt - Do nothing for stray interrupts.
*
* Do nothing for stray interrupts.
*/

void sysStrayInt (void)
    {
    sysStrayIntCount++;
    }

/*******************************************************************************
*
* sysMmuMapAdd - insert a new MMU mapping
*
* This routine will create a new <sysPhysMemDesc> table entry for a memory
* region of specified <length> in bytes and with a specified base
* <address>.  The <initialStateMask> and <initialState> parameters specify
* a PHYS_MEM_DESC type state mask and state for the memory region.
*
* CAVEATS
* This routine must be used before the <sysPhysMemDesc> table is
* referenced for the purpose of initializing the MMU or processor address
* space (us. in usrMmuInit()).
*
* The <length> in bytes will be rounded up to a multiple of VM_PAGE_SIZE
* bytes if necessary.
*
* The current implementation assumes a one-to-one mapping of physical to
* virtual addresses.
*
* If the current memory region contains some pages which had been mapped
* with different attributes previously, this routine will do nothing and
* return an ERROR directly.
*
* RETURNS: OK or ERROR depending on availability of free mappings.
*
* SEE ALSO: vmLib
*/

STATUS sysMmuMapAdd
    (
    void * address,           /* memory region base address */
    UINT   length,            /* memory region length in bytes*/
    UINT   initialStateMask,  /* PHYS_MEM_DESC state mask */
    UINT   initialState       /* PHYS_MEM_DESC state */
    )
    {
    PHYS_MEM_DESC * pMmu;
    PHYS_MEM_DESC * pPmd;

    UINT32   pageAddress = 0;
    UINT32   pageLength = 0;
    UINT32   pageAddressTop = 0;
    UINT32   pageLengthTop = 0;
    UINT32   pageAddressBottom = 0;
    UINT32   pageLengthBottom = 0;

    UINT32   pageStart;
    UINT32   pageEnd;
    UINT32   pageCount;

    UINT32   mapStart;
    UINT32   mapEnd;

    BOOL     pageNotMapped = FALSE;
    BOOL     noOverlap = FALSE;
    BOOL     topOverlap = FALSE;
    BOOL     bottomOverlap = FALSE;

    UINT32   i;

    STATUS   result = OK;

    mapStart = 0;
    mapEnd   = 0;

    /* Calculate(align) the start/end of page address, and the count of pages */

    pageLength = ROUND_UP (length, VM_PAGE_SIZE);

    pageStart = ROUND_DOWN (address, VM_PAGE_SIZE);
    pageEnd   = ((pageStart + (pageLength - 1)) / VM_PAGE_SIZE) * VM_PAGE_SIZE;
    pageCount = (pageEnd - pageStart) / VM_PAGE_SIZE + 1;

    pageAddress  = pageStart;

    for (i=0; i<sysPhysMemDescNumEnt; i++)
        {
        pageNotMapped = TRUE;
        noOverlap = TRUE;
        topOverlap = FALSE;
        bottomOverlap = FALSE;

        pPmd = &sysPhysMemDesc[i];

        mapStart = (UINT32) pPmd->virtualAddr;
        mapEnd   = (UINT32) pPmd->virtualAddr + pPmd->len - VM_PAGE_SIZE;

        if(pageStart >= mapStart && pageEnd <= mapEnd)
            {
            /* Mapping fully contained */

            if (pPmd->initialStateMask != initialStateMask  ||
                pPmd->initialState != initialState  ||
                pPmd->virtualAddr != pPmd->physicalAddr)
                {
                result = ERROR; /* Attributes are different */
                break;
                }

            noOverlap = FALSE;
            pageNotMapped = FALSE;
            break;
            }

        if((pageStart >= mapStart) && (pageEnd > mapEnd) &&
           (pageStart < mapEnd))
            {
            /* Top Overlap */

            if (pPmd->initialStateMask != initialStateMask  ||
                pPmd->initialState != initialState  ||
                pPmd->virtualAddr != pPmd->physicalAddr)
                {
                result = ERROR; /* Attributes are different */
                break;
                }

            /* Need to add entry for (pageEnd-mapEnd) */

            topOverlap = TRUE;
            pageAddressTop = (mapEnd + VM_PAGE_SIZE);
            pageLengthTop = (pageEnd - mapEnd);
            }

        if((pageStart < mapStart) && (pageEnd <= mapEnd) &&
           (pageEnd > mapStart))
            {
            /* Bottom Overlap */

            if (pPmd->initialStateMask != initialStateMask  ||
                pPmd->initialState != initialState  ||
                pPmd->virtualAddr != pPmd->physicalAddr)
                {
                result = ERROR; /* Attributes are different */
                break;
                }

            /* Need to add entry for (mapStart-pageStart) */

            bottomOverlap = TRUE;
            pageAddressBottom = pageStart;
            pageLengthBottom = (mapStart-pageStart);
            }

        if(topOverlap || bottomOverlap)
            break;

        }

    if (pageNotMapped && (result != ERROR))
        {
        pMmu = &sysPhysMemDesc[sysPhysMemDescNumEnt];
        if (pMmu->virtualAddr != (VIRT_ADDR) DUMMY_VIRT_ADDR)
            {
            result = ERROR;
            }
        else
            {
            if(noOverlap)
                {
                pMmu->virtualAddr       = (VIRT_ADDR)pageAddress;
                pMmu->physicalAddr      = (PHYS_ADDR)pageAddress;
                pMmu->len               = pageLength;
                pMmu->initialStateMask  = initialStateMask;
                pMmu->initialState      = initialState;
                sysPhysMemDescNumEnt    += 1;
                }

            if(topOverlap)
                {
                pMmu->virtualAddr       = (VIRT_ADDR)pageAddressTop;
                pMmu->physicalAddr      = (PHYS_ADDR)pageAddressTop;
                pMmu->len               = pageLengthTop;
                pMmu->initialStateMask  = initialStateMask;
                pMmu->initialState      = initialState;
                sysPhysMemDescNumEnt    += 1;
                }

            if(bottomOverlap)
                {
                pMmu->virtualAddr       = (VIRT_ADDR)pageAddressBottom;
                pMmu->physicalAddr      = (PHYS_ADDR)pageAddressBottom;
                pMmu->len               = pageLengthBottom;
                pMmu->initialStateMask  = initialStateMask;
                pMmu->initialState      = initialState;
                sysPhysMemDescNumEnt    += 1;
                }
            }
        }

    return (result);
    }

#if defined(DRV_SIO_NS16550)
SIO_CHAN * bspSerialChanGet
    (
    int channel     /* serial channel */
    )
    {
    return ((SIO_CHAN *) ERROR);
    }
#endif /* defined(DRV_SIO_NS16550) */

#if defined (INCLUDE_VXBUS)
#if defined(INCLUDE_PC_CONSOLE) && defined (DRV_TIMER_I8253)
/*******************************************************************************
*
* sysConBeep - sound beep function (using timer 2 for tone)
*
* This function is responsible for producing the beep
*
* RETURNS: N/A
*
* \NOMANUAL
*/

LOCAL void sysConBeep
    (
    BOOL    mode    /* TRUE:long beep  FALSE:short beep */
    )
    {
    int     beepTime;
    int     beepPitch;
    FAST int    oldlevel;

    if (mode)
        {
        beepPitch = BEEP_PITCH_L;
        beepTime  = BEEP_TIME_L;    /* long beep */
        }
    else
        {
        beepPitch = BEEP_PITCH_S;
        beepTime  = BEEP_TIME_S;    /* short beep */
        }

    oldlevel = intCpuLock ();

    /* set command for counter 2, 2 byte write */

    sysOutByte(PIT_BASE_ADR + 3, (char)0xb6);
    sysOutByte(PIT_BASE_ADR + 2, (beepPitch & 0xff));
    sysOutByte(PIT_BASE_ADR + 2, (beepPitch >> 8));

    /* enable counter 2 */

    sysOutByte(DIAG_CTRL, sysInByte(DIAG_CTRL) | 0x03);

    taskDelay (beepTime);

    /* disable counter 2 */

    sysOutByte(DIAG_CTRL, sysInByte(DIAG_CTRL) & ~0x03);

    intCpuUnlock (oldlevel);
    return;
    }
#endif /* INCLUDE_PC_CONSOLE && DRV_TIMER_I8253 */
#endif /* INCLUDE_VXBUS && DRV_TIMER_I8253 */


/******************************************************************************
*
* sysHpetOpen - map and initialize the HPET hardware interface.
*
* This function supplied to vxbIntelTimestamp driver that make the driver being
* able to use the HPET timer hardware in its first level initialization.
*
* RETURNS: OK or ERROR depending on supported of HPET.
*/

STATUS sysHpetOpen(void)
    {
#if defined (INCLUDE_HPET_TIMESTAMP)
    return OK;
#else  /* defined (INCLUDE_HPET_TIMESTAMP) */
    return ERROR;
#endif /* defined (INCLUDE_HPET_TIMESTAMP) */
    }

#if defined (_WRS_CONFIG_SMP)
/******************************************************************************
*
* sysCpuAvailableGet - return the number of CPUs that are to
*                      be used by vxWorks for the SMP kernel.
*
* RETURNS: The number for the CPUs available
*
*/

UINT32 sysCpuAvailableGet(void)
    {
    /* enforce arch limitation  for SMP configurable cores */

    return ((UINT32) (vxCpuCount));
    }

/*******************************************************************************
*
* vxCpuStateInit - Initialize the state to start a CPU in.
*
* The purpose of this function is somewhat similar to the arch-specific
* function taskRegsInit(), except that this function initializes a
* WIND_CPU_STATE structure suitable for the initial state of a processor,
* whereas taskRegsInit only needs to initialize a REG_SET structure
* for the initial state of a vxWorks task.
*
* RETURNS: OK or ERROR if any failure.
*
*/

STATUS vxCpuStateInit
    (
    unsigned int cpu,
    WIND_CPU_STATE *cpuState,
    char *stackBase,
    FUNCPTR entry
    )
    {
    TSS *newTss;
    GDT *newGdt;

    GDTPTR gDtp;
    INT32 nGdtEntries, nGdtBytes;

    if ((cpu < 1) || (cpu > (sysCpuAvailableGet() - 1)))
        return (ERROR);

    if ((cpuState == NULL) || (entry== NULL) || (stackBase == NULL))
        return (ERROR);

    bzero ((char *)cpuState, sizeof(WIND_CPU_STATE));

        /*
         * establish pointers to global descriptor table & task state segment
         * TSS is assigned a cpu unique structure...
         */

        /* copy the global descriptor table from RAM/ROM to RAM */

        /* Get the Global Data Table Descriptor */

        vxGdtrGet ( (long long int *)&gDtp );

        /* Extract the number of bytes */

        nGdtBytes = (INT32)gDtp.wGdtr[0];

        /* and calculate the number of entries */

        nGdtEntries = (nGdtBytes + 1 ) / sizeof(GDT);

        newGdt = (GDT *) malloc (nGdtEntries * sizeof (GDT));

        if (newGdt == NULL)
            return (ERROR);

        bcopy ((char *)(&sysGdt), (char *)newGdt,
            (int)(nGdtEntries * sizeof (GDT)));

        _WRS_WIND_VARS_ARCH_SET(cpu,
                            pSysGdt,
                            newGdt);

        newTss = (TSS *) malloc (sizeof (TSS));

        if (newTss == NULL)
            return (ERROR);

        bcopy ((char *)(&sysTss), (char *)newTss, (int)sizeof (TSS));

        /*
         * Update vxKernelVars
         */

        _WRS_WIND_VARS_ARCH_SET(cpu,
                            sysTssCurrent,
                            newTss);

        _WRS_WIND_VARS_ARCH_SET(cpu,
                            vxIntStackEnabled,
                            1);

    cpuState->regs.eflags = EFLAGS_BRANDNEW;
    cpuState->regs.esp = (ULONG) stackBase;
    cpuState->regs.pc = (INSTR *)entry;

    return (OK);
    }

/***************************************************************************
*
* sysCpuStart - start a CPU
*
* The sysCpuStart() function takes two parameters:
* int cpu;                  /@ core number to start @/
* WIND_CPU_STATE *cpuState; /@ pointer to a WIND_CPU_STATE structure @/
*
* The intent is to implement a function with the basic features of
* vxTaskRegsInit() that sets up the regs structure, then passes the
* cpu number and cpuState pointer to this function, which in turn extracts
* the needed values from the cpuState structure and starts the processor
* running...
*
* RETURNS:  0K - Success
*           ERROR - Fail
*
* \NOMANUAL
*/

STATUS sysCpuStart
    (
    int cpu,
    WIND_CPU_STATE *cpuState
    )
    {
    STATUS status = OK;          /* return value */
    INT32  ix;
    INT32  oldLevel;
    UINT32 oldResetVector;       /* warm reset vector */
    UINT8  oldShutdownCode;      /* CMOS shutdown code */
    unsigned int *tmp_stk;       /* temp stk to pass some env params */
    unsigned int *tmpPtr;        /* temp pointer */
    FUNCPTR * baseAddr;          /* new vector base address */
    UINT8 *mploApicIndexTable;
    char * textLoc;
    UINT apDelayCount = SYS_AP_LOOP_COUNT; /* Times to check for AP startup */
    UINT apDelay = SYS_AP_TIMEOUT;         /* Time between AP startup checks */

    /* entry point of AP trampoline code */

    UINT32 entryPoint = (UINT32) CPU_ENTRY_POINT;
    UINT32 scratchPoint = (UINT32) CPU_SCRATCH_POINT;
    UINT32 scratchMem = (UINT32) scratchPoint;

    (*getMpApicLoIndexTable)(mpApicDevID, (void *)&mploApicIndexTable);

    textLoc = (char *)&sysInitCpuStartup;

        /*
         * Copy tramp code to low memory. Must be reachable
         * by 16 bit code.
         */

    bcopy (textLoc, (char *)entryPoint, (int)CPU_AP_AREA);

    /*
     * Initialization of Temporary AP Scratch Memory:
     *
     *    scratchMem (scratch memory offset)  scratchPoint
     *
     *    Standard GDT Entries:
     *
     *    null descriptor                     scratchMem + 0x04
     *
     *    kernel text descriptor              scratchMem + 0x0C
     *
     *    kernel data descriptor              scratchMem + 0x14
     *
     *    exception text descriptor           scratchMem + 0x1C
     *
     *    interrupt data descriptor           scratchMem + 0x24
     *
     *    gdt limit << 16                     scratchMem + LIM_GDT_OFFSET
     *    gdt base                            scratchMem + BASE_GDT_OFFSET
     *    address of page directory           scratchMem + CR3_OFFSET
     *
     *    idt limit                           scratchMem + LIM_IDT_OFFSET
     *    idt base address                    scratchMem + BASE_IDT_OFFSET
     *
     *    initial AP stack addr               scratchMem + AP_STK_OFFSET
     */

    /* Setup Initial AP... */

    /*
     * scratchPoint saves location of starting point for AP scratch memory,
     * APs offset based on this value for unique memory space...
     * if AP #1, this location doubles as first valid location of AP
     * scratch memory...
     */

    *((int *)(scratchPoint)) = (unsigned int)scratchMem;

    /*
     * first valid location of scratch memory,
     * setup offset for AP specific scratch mem
     */

    tmp_stk = (unsigned int *)(scratchMem);
    tmp_stk[0] = (unsigned int)scratchMem;

    /* setup intial GDT values */

    tmpPtr = (unsigned int *) &sysGdt;

    /* place tmp_gdt in memory */

    for (ix = 0; ix < NUM_GDT_ENTRIES; ix++)
        tmp_stk[ix+1] = *tmpPtr++;

    /* (num bytes - 1) << 16 */

    *((int *)(scratchMem + LIM_GDT_OFFSET)) = (int)0x270000;
    *((int *)(scratchMem + BASE_GDT_OFFSET)) = (int)(scratchMem + 0x04);

        /* setup initial cr3 */

        *((unsigned int *)(scratchMem + CR3_OFFSET)) = vxCr3Get();

        /* setup initial IDT values */

        baseAddr = intVecBaseGet();

    /* (IDT limit) << 16 */

    *((int *)(scratchMem + LIM_IDT_OFFSET)) = (int)0x07ff0000;

    /* IDT base address */

    *((int *)(scratchMem + BASE_IDT_OFFSET)) = (int)baseAddr;

    /* initial AP Stack Address */

        *((int *)(scratchMem + AP_STK_OFFSET)) = (int)cpuState->regs.esp;

    oldLevel = intCpuLock();               /* LOCK INTERRUPTS */

    /*
     * allow access to page 0, for access to trampoline region
     * default state (VM_STATE_VALID | VM_STATE_WRITABLE | VM_STATE_CACHEABLE)
     */

    if (vmMap (0, (VIRT_ADDR)0, (PHYS_ADDR)0, VM_PAGE_SIZE) == ERROR)
        status = ERROR;

    /* set the AP entry point address in WARM_REST_VECTOR */

    oldResetVector = *(volatile UINT32 *)WARM_RESET_VECTOR;
    sysOutByte (RTC_INDEX,
                BIOS_SHUTDOWN_STATUS); /* selects Shutdown Status Register */
    oldShutdownCode = sysInByte (RTC_DATA); /* get BIOS Shutdown code */

    *(volatile unsigned short *)WARM_RESET_VECTOR = 0;
    *(volatile unsigned short *)(WARM_RESET_VECTOR+2) = (entryPoint>>4);

    /* initialze the BIOS shutdown code to be 0xA */

    sysOutByte (RTC_INDEX,
                BIOS_SHUTDOWN_STATUS); /* selects Shutdown Status Register */
    sysOutByte (RTC_DATA, 0xA);             /* set BIOS Shutdown code to 0x0A */

    intCpuUnlock(oldLevel);                 /* UNLOCK INTERRUPTS */

#ifdef _WRS_VX_IA_IPI_INSTRUMENTATION
    /* Zero IPI instrumentation for this CPU */

    bzero((char *)ipiCounters[cpu], sizeof(struct ipiCounters));
#endif /* _WRS_VX_IA_IPI_INSTRUMENTATION */

    cacheI86Flush();

    /* BSP sends AP an INIT-IPI and STARTUP-IPI */

    if (ipiStartup ((UINT32) mploApicIndexTable[cpu],
                    (UINT32) entryPoint,
                    (UINT32) 2) != OK)
        {
        printf ("\nipiStartup failed: %d\n", mploApicIndexTable[cpu]);
        status = ERROR;
        }

   /*
    * Wait for AP start up.
    * We do not always have to delay. When a processor is enabled it is
    * visible via vxCpuEnabled.
    */
   
   do {
       if ((1 << cpu) & vxCpuEnabled)
           break;
       else
           sysUsDelay (apDelay);

   } while (--apDelayCount > 0);

   /*
    * Set ERROR when AP did not start in time.
    * tRootTask will log CPU startup error to the console device.
    */

   if (((1 << cpu) & vxCpuEnabled) == 0)
       status = ERROR;

    oldLevel = intCpuLock();                /* LOCK INTERRUPTS */

    /* restore the WARM_REST_VECTOR and the BIOS shutdown code */

    *(volatile UINT32 *)WARM_RESET_VECTOR = oldResetVector;
    sysOutByte (RTC_INDEX, BIOS_SHUTDOWN_STATUS); /* Shutdown Status Reg */
    sysOutByte (RTC_DATA, oldShutdownCode); /* set BIOS Shutdown code */

   cacheI86Flush();

   intCpuUnlock(oldLevel);                 /* UNLOCK INTERRUPTS */

    /* unmap the page zero for NULL pointer detection */

   vmPageUnmap((VM_CONTEXT_ID)0, (VIRT_ADDR)0, (size_t)VM_PAGE_SIZE);

   return (status);

    }

/******************************************************************************
*
* sysCpuInit - AP CPU Initialization.
*
* This routine is the first code executed on a secondary processor after
* startup. It is responsible for ensuring that the registers are set
* appropriately.
*
* In a mapped kernel, it is also responsible for setting up the core's MMU.
*
* Upon completion of these tasks, the core is ready to begin accepting
* kernel work. The address of the initial kernel code (typically
* windCpuEntry) has been placed in sysCpuInitTable[coreNum].
*
* RETURNS: NONE
*/

void sysCpuInit (void)
    {
    unsigned int a = 0;
    int cpuNum;
    FUNCPTR entry;
    int dummy;
    STATUS retVal = OK;

    UINT64 cpuKernelVars;
    unsigned int cpuIndex;

    sysInitGDT ();

    /* establish dedicated segment register for vxKernelVars */

    cpuIndex = vxCpuIndexGet();
    vxKernelVars[cpuIndex].vars.cpu_archVars.cpu_cpuIndex = cpuIndex;
    cpuKernelVars = (UINT64) &vxKernelVars[cpuIndex];
    vxKernelVars[cpuIndex].vars.cpu_archVars.cpu_cpuGsBaseCurrent =
            (UINT32) &vxKernelVars[cpuIndex];
    pentiumMsrSet (IA32_GS_BASE, &cpuKernelVars);

    /* enable the MTRR (Memory Type Range Registers) */

    if ((vxCpuId.std.featuresEdx & CPUID_MTRR) == CPUID_MTRR)
        {
        pentiumMtrrDisable ();      /* disable MTRR */
#ifdef INCLUDE_MTRR_GET
        (void) pentiumMtrrGet (&sysMtrr); /* get MTRR initialized by BIOS */
#else
        (void) pentiumMtrrSet (&sysMtrr); /* set your own MTRR */
#endif /* INCLUDE_MTRR_GET */
        pentiumMtrrEnable ();       /* enable MTRR */
        }

    /* enable FP features */

    fppArchCpuInit();
    
#ifdef INCLUDE_PMC

    /* enable PMC (Performance Monitoring Counters) */

    pentiumPmcStop ();          /* stop PMC0 and PMC1 */
    pentiumPmcReset ();         /* reset PMC0 and PMC1 */
#endif /* INCLUDE_PMC */

    /* enable the MCA (Machine Check Architecture) */

    pentiumMcaEnable (TRUE);

    (*getLoApicIntrInitAP)(loApicDevID, (void *) &dummy);

    cpuNum = vxCpuPhysIndexGet();

    entry = sysCpuInitTable[cpuNum];

    /*
     * update GDT. LDT, ect. depending on configuration...
     *
     * Note: order here is important, GDT updates for RTPs
     *       must come before OSM support, if included...
     */

#if defined (INCLUDE_RTP)
    retVal = syscallArchInit ();
#endif /* INCLUDE_RTP */

    if (retVal == OK)
        {
#if defined INCLUDE_PROTECT_TASK_STACK ||       \
    defined INCLUDE_PROTECT_INTERRUPT_STACK
#if defined(_WRS_OSM_INIT)
#if defined INCLUDE_PROTECT_TASK_STACK
        retVal = excOsmInit (TASK_USER_EXC_STACK_OVERFLOW_SIZE, VM_PAGE_SIZE);
#else  /* INCLUDE_PROTECT_TASK_STACK */
        retVal = excOsmInit (0, VM_PAGE_SIZE);
#endif /* INCLUDE_PROTECT_TASK_STACK */
#endif /* defined(_WRS_OSM_INIT) */
#endif  /* INCLUDE_PROTECT_TASK_STACK || INCLUDE_PROTECT_INTERRUPT_STACK */

        if ((entry != NULL) && (retVal == OK))
            {
            entry ();

            /* should be no return */

            }
        }

    for (;;)
        {
        if (!(++a % 0x10000))
            {
            sysCpuLoopCount[cpuNum]++;
            }
        }
    }

/******************************************************************************
*
* sysCpuEnable - enable a multi core CPU
*
* This routine brings a CPU out of reset
*
* RETURNS:  OK - Success
*           ERROR - Fail
*
*/

STATUS sysCpuEnable
    (
    unsigned int cpuNum,
    WIND_CPU_STATE *cpuState
    )
    {
    if ((cpuNum < 1) || (cpuNum > (sysCpuAvailableGet() - 1)))
        {
        return (ERROR);
        }

        sysCpuInitTable[cpuNum] = (FUNCPTR) cpuState->regs.pc;

    return (sysCpuStart(cpuNum, cpuState));
    }

/******************************************************************************
*
* sysCpuRe_Enable - re-enable a multi core CPU
*
* This routine brings an AP CPU through a cpu reset cycle and back on line.
*
* RETURNS:  OK - Success
*           ERROR - Fail
*
*/

STATUS sysCpuRe_Enable
    (
    unsigned int cpuNum
    )
    {
    STATUS status = OK;

    if ((cpuNum < 1) || (cpuNum > (sysCpuAvailableGet() - 1)))
        {
        return (ERROR);
        }

    status = sysCpuStop ((int)cpuNum);
    status = sysCpuReset ((int)cpuNum);
    status = kernelCpuEnable (cpuNum);

    return (status);
    }

/******************************************************************************
*
* sysCpuDisable - disable a multi core CPU
*
* This routine shuts down the specified core.
*
* RETURNS:  OK - Success
*           ERROR - Fail
*
*/

STATUS sysCpuDisable
    (
    int cpuNum
    )
    {
    if ((cpuNum < 1) || (cpuNum > (sysCpuAvailableGet() - 1)))
        {
        return (ERROR);
        }

    return (sysCpuStop(cpuNum));
    }

/***************************************************************************
*
* sysCpuStop - stop a CPU, for now, it is no longer available
*              for use
*
* The sysCpuStop() function takes one parameter:
* int cpu;              /@ core number to stop @/
*
* RETURNS: OK
*
* \NOMANUAL
*/

STATUS sysCpuStop
    (
    int cpu
    )
    {
    STATUS status = OK;
    int key;   /* prevent task migration */
    UINT8 *mploApicIndexTable;

    taskCpuLock();       /* if not in interrupt context, taskCpuLock */

    key = intCpuLock ();   /* prevent task migration */

    (*getMpApicLoIndexTable)(mpApicDevID, (void *)&mploApicIndexTable);

    /* BSP sends AP the shutdown IPI */

    CPUSET_ATOMICCLR (vxCpuEnabled, CPU_PHYS_TO_LOGICAL(cpu));
    status = ipiShutdown ((UINT32) mploApicIndexTable[cpu],
                          INT_NUM_LOAPIC_IPI + 0);

    intCpuUnlock (key);

    taskCpuUnlock();     /* if not in interrupt context, taskCpuUnlock */

    return (status);
    }

/***************************************************************************
*
* sysCpuStop_APs - Stop all Application Processors (APs)
*
* Can only run this routine from boot strap processor (BSP)
*
* RETURNS:  OK - Success
*           ERROR - Fail
*
* \NOMANUAL
*/

STATUS sysCpuStop_APs (void)
    {
    STATUS status = OK;
    int i;
    int key;   /* prevent task migration */
    UINT8 *mploApicIndexTable;
    int numCpus = vxCpuConfiguredGet();

    taskCpuLock();       /* if not in interrupt context, taskCpuLock */

    key = intCpuLock ();   /* prevent task migration */

    (*getMpApicLoIndexTable)(mpApicDevID, (void *)&mploApicIndexTable);

    /* Skip BSP, i == 0 */

    for (i=(vxCpuPhysIndexGet()+1); i < numCpus; i++)
        {
        /* BSP sends AP the shutdown IPI */

        CPUSET_ATOMICCLR (vxCpuEnabled, CPU_PHYS_TO_LOGICAL(i));
        status = ipiShutdown ((UINT32) mploApicIndexTable[i],
                              INT_NUM_LOAPIC_IPI + 0);
        }

    intCpuUnlock (key);

    taskCpuUnlock();     /* if not in interrupt context, taskCpuUnlock */

    return (status);
    }

/***************************************************************************
*
* sysCpuStop_ABM - Stop all APs but the one I'm running on, (All but me...)
*
* RETURNS:  OK - Success
*           ERROR - Fail
*
* \NOMANUAL
*/

STATUS sysCpuStop_ABM (void)
    {
    STATUS status = OK;
    int i, skipProc;
    int key;   /* prevent task migration */
    UINT8 *mploApicIndexTable;
    int numCpus = vxCpuConfiguredGet();

    taskCpuLock();       /* if not in interrupt context, taskCpuLock */

    key = intCpuLock ();   /* prevent task migration */

    (*getMpApicLoIndexTable)(mpApicDevID, (void *)&mploApicIndexTable);

    skipProc = vxCpuPhysIndexGet ();

    for (i=1; i < numCpus; i++)
        {
        if (i != skipProc)
            {
            /* sends AP the shutdown IPI */

            CPUSET_ATOMICCLR (vxCpuEnabled, CPU_PHYS_TO_LOGICAL(i));
            status = ipiShutdown ((UINT32) mploApicIndexTable[i],
                                  INT_NUM_LOAPIC_IPI + 0);
            }
        }

    intCpuUnlock (key);

    taskCpuUnlock();     /* if not in interrupt context, taskCpuUnlock */

    return (status);
    }

/***************************************************************************
*
* sysCpuReset - Reset a CPU
*
* Places the specified Application Processor (AP) into the
* INIT reset state, i.e. wait-for-SIPI state.
*
* The sysCpuReset() function takes one parameter:
* int cpu;              /@ core number to stop @/
*
* RETURNS: OK
*
* \NOMANUAL
*/

STATUS sysCpuReset
    (
    int cpu
    )
    {
    STATUS status = OK;
    int key;   /* prevent task migration */
    UINT8 *mploApicIndexTable;

    taskCpuLock();       /* if not in interrupt context, taskCpuLock */

    key = intCpuLock ();   /* prevent task migration */

    (*getMpApicLoIndexTable)(mpApicDevID, (void *)&mploApicIndexTable);

    /* sends AP the INIT IPI */

    CPUSET_ATOMICCLR (vxCpuEnabled, cpu);
    status = ipiReset ((UINT32) mploApicIndexTable[cpu]);

    intCpuUnlock (key);

    taskCpuUnlock();     /* if not in interrupt context, taskCpuUnlock */

    return (status);
    }

/***************************************************************************
*
* sysCpuReset_APs - Reset all Application Processors (APs)
*
* Can only run this routine from BSP.
*
* RETURNS:  OK - Success
*           ERROR - Fail
*
* \NOMANUAL
*/

STATUS sysCpuReset_APs (void)
    {
    STATUS status = OK;
    int i, pCpu;
    int key;   /* prevent task migration */
    UINT8 *mploApicIndexTable;
    int numCpus = vxCpuConfiguredGet();

    pCpu = vxCpuPhysIndexGet();

    taskCpuLock();       /* if not in interrupt context, taskCpuLock */

    key = intCpuLock ();   /* prevent task migration */

    (*getMpApicLoIndexTable)(mpApicDevID, (void *)&mploApicIndexTable);

    /* Skip BSP, i == 0 */

    for (i = 1; i < numCpus; i++)
        {
        /* BSP sends AP the INIT IPI */

        CPUSET_ATOMICCLR (vxCpuEnabled, CPU_PHYS_TO_LOGICAL(pCpu + i));
        status = ipiReset ((UINT32) mploApicIndexTable[pCpu + i]);
        }

    intCpuUnlock (key);

    taskCpuUnlock();     /* if not in interrupt context, taskCpuUnlock */

    return (status);
    }

/***************************************************************************
*
* sysCpuReset_ABM - Reset all APs but the one I'm running on, (All but me...)
*
* RETURNS:  OK - Success
*           ERROR - Fail
*
* \NOMANUAL
*/

STATUS sysCpuReset_ABM (void)
    {
    STATUS status = OK;
    int i, skipProc;
    int key;   /* prevent task migration */
    UINT8 *mploApicIndexTable;
    int numCpus = vxCpuConfiguredGet();

    taskCpuLock();       /* if not in interrupt context, taskCpuLock */

    key = intCpuLock ();   /* prevent task migration */

    (*getMpApicLoIndexTable)(mpApicDevID, (void *)&mploApicIndexTable);

    skipProc = vxCpuPhysIndexGet ();

    for (i=1; i < numCpus; i++)
        {
        if (i != skipProc)
            {
            /* sends AP the INIT IPI */

            CPUSET_ATOMICCLR (vxCpuEnabled, CPU_PHYS_TO_LOGICAL(i));
            status = ipiReset ((UINT32) mploApicIndexTable[i]);
            }
        }

    intCpuUnlock (key);

    taskCpuUnlock();     /* if not in interrupt context, taskCpuUnlock */

    return (status);
    }

#endif /*  (_WRS_CONFIG_SMP) */

/* Static configuration of mpApic data table */

#if defined (INCLUDE_VIRTUAL_WIRE_MODE) || defined (INCLUDE_SYMMETRIC_IO_MODE)

/*
 * Static configuration table for mpApicData structure,
 * allows for manual configuration of mpApic driver.
 *
 * One must be very careful when manually configuring Apics,
 * incorrect data will cause catastrophic error, i.e. BSP
 * failing to boot properly.
 *
 * The data here is dependent on the number of cores involved,
 * interrupt routing tables, basically the general configuration
 * of your specific hardware.
 *
 * Access to this capability is obtained by defining INCLUDE_USR_BOOT_OP
 * in config.h or by adding the INCLUDE_USR_BOOT_OP component.
 *
 * created for itl_hanlanCreek using ACPI 9/22/09
 * BIOS version 1.23.1114 American Megatrends
 *
 * NB: The table below may not work with other BIOS versions
 */

/* mpApicAddrTable table */


static UINT32 staticMpApicAddrTable [] =
    {
    0xfec00000, 0xfec90000
    };


/* mpApicLogicalTable table */


static UINT32 staticMpApicLogicalTable [] =
    {
    0x00030908
    };


/* Interrupt Routing Table */


static UINT32 staticMpApicInterruptTable [] =
    {
    0x00000003, 0x00080008, 0x00000003, 0x01080108,
    0x00000003, 0x02080008, 0x00000003, 0x03080308,
    0x00000003, 0x04080408, 0x00000003, 0x05080508,
    0x00000003, 0x06080608, 0x00000003, 0x07080708,
    0x00000003, 0x08080808, 0x000d0003, 0x09080908,
    0x00000003, 0x0a080a08, 0x00000003, 0x0b080b08,
    0x00000003, 0x0c080c08, 0x00000003, 0x0d080d08,
    0x00000003, 0x0e080e08, 0x00000003, 0x0f080f08,
    0x00000003, 0x1c080001, 0x00000003, 0x28080101,
    0x00000003, 0x29080201, 0x00000003, 0x2f080301,
    0x00000003, 0x18080002, 0x00000003, 0x22080102,
    0x00000003, 0x23080202, 0x00000003, 0x24080302,
    0x00000003, 0x1a080003, 0x00000003, 0x19080103,
    0x00000003, 0x1b080203, 0x00000003, 0x1d080303,
    0x00000003, 0x1e080004, 0x00000003, 0x25080104,
    0x00000003, 0x27080204, 0x00000003, 0x26080304,
    0x00000003, 0x20080005, 0x00000003, 0x2a080105,
    0x00000003, 0x2f080205, 0x00000003, 0x29080305,
    0x00000003, 0xf8090006, 0x00000003, 0xf9090106,
    0x00000003, 0xfa090206, 0x00000003, 0xfb090306,
    0x00000000, 0x00000000, 0x00000003, 0xf8090008,
    0x00000003, 0xf9090108, 0x00000003, 0xfa090208,
    0x00000003, 0xfb090308, 0x00000304, 0x00ff0000,
    0x00000104, 0x01ff0000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x43500101, 0x00000049,
    0x4350ff01, 0x00000049, 0x43500201, 0x00000049,
    0x43500301, 0x00000049, 0x43500401, 0x00000049,
    0x43500501, 0x00000049, 0x43500601, 0x00000049,
    0x43500801, 0x00000049, 0x53490901, 0x00000041,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000
    };


/* mpApicBusTable table */


static UINT32 staticMpApicBusTable [] =
    {
    0x43500101, 0x00000049, 0x4350ff01, 0x00000049,
    0x43500201, 0x00000049, 0x43500301, 0x00000049,
    0x43500401, 0x00000049, 0x43500501, 0x00000049,
    0x43500601, 0x00000049, 0x43500801, 0x00000049,
    0x53490901, 0x00000041 };

static UINT32 staticMploApicIndexTable [] =
    {
    0x12021000, 0x16061404
    };

static UINT32 staticMpApicData[] =
    {
    /* instance pointer */

    0x00000000,

    /* mem location of MP_APIC_DATA */

    0x00102000,

    /* size of MP_APIC_DATA */

    0x000001a4,

    /* NONE/MP/ACPI/USR boot structure used */

    0x00000003,

    /* MP Floating Pointer Structure */

    0x5f504d5f, 0x00000000, 0x00000000, 0x00000000,

    /* MP Configuration Table Header */

    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000,

    /* def Local APIC addr */

    0xfee00000,

    /* number of IO APICs (MP Table) */

    0x00000002,

    /* logical io apic id to address mapping */

    (UINT32)staticMpApicAddrTable,

    /* mem location of mpApicAddrTable */

    (UINT32)staticMpApicAddrTable,

    /* size of mpApicAddrTable */

    0x00000008,

    /* recorded id mapping */

    (UINT32)staticMpApicLogicalTable,

    /* mem location of mpApicLogicalTable */

    (UINT32)staticMpApicLogicalTable,

    /* size of mpApicLogicalTable */

    0x00000004,

    /* number of io interrupts (MP Table) */

    0x0000002d,

    /* number of local interrupts (MP Table) */

    0x00000012,

    /* interrupt routing table */

    (UINT32)staticMpApicInterruptTable,

    /* mem location of mpApicInterruptTable */

    (UINT32)staticMpApicInterruptTable,

    /* size of mpApicInterruptTable */

    0x000001f8,

    /* number of buses (MP Table) */

    0x00000009,

    /* bus routing table */

    (UINT32)staticMpApicBusTable,

    /* mem location of mpApicBusTable */

    (UINT32)staticMpApicBusTable,

    /* size of mpApicBusTable */

    0x00000048,

    /* loApic Id translation */

    (UINT32)staticMploApicIndexTable,

    /* mem location of mploApicIndexTable */

    (UINT32)staticMploApicIndexTable,

    /* size of mploApicIndexTable */

    0x00000008,


    /* cpu Id translation --Reserved for non-SMP images */

    0x00020000, 0x00060004, 0x00000000, 0x00000000,
    0x00030001, 0x00070005, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,

    /* CPU counter --Reserved for non-SMP images */

    0x00000008,

    /* Boot Strap Processor localApicId */

    0x00000000

    };

/*******************************************************************************
*
* sysStaticMpApicDataGet - Copies static MpApic data into table
*
* Copies static MpApic data into table pointed to by pMpApicTable.
*
* The sysStaticMpApicDataGet() function takes one parameter:
* MP_APIC_DATA *pMpApicTable; /@ pointer to the MpApic Table @/
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void sysStaticMpApicDataGet
    (
    MP_APIC_DATA *pMpApicTable
    )
    {
    INT32 oldLevel = intCpuLock();  /* LOCK INTERRUPTS */

    /* initialize the memory allocated */

    bzero((char *)pMpApicTable, sizeof(MP_APIC_DATA));

    /*
     * mpApicData structure - structure holding mpApic details retrieved from
     *                        BIOS/ACPI/or USER defined methods.
     */

    bcopy ((char *) staticMpApicData,
           (char *) pMpApicTable,
           (int) sizeof (MP_APIC_DATA));

    /* logical io apic id to address mapping */

    bcopy ((char *) staticMpApicAddrTable,
           (char *) pMpApicTable->mpApicAtLoc,
           (int) pMpApicTable->mpApicAtSize);

    /* recorded id mapping */

    bcopy ((char *) staticMpApicLogicalTable,
           (char *) pMpApicTable->mpApicLtLoc,
           (int) pMpApicTable->mpApicLtSize);

    /* interrupt routing table */

    bcopy ((char *) staticMpApicInterruptTable,
           (char *) pMpApicTable->mpApicItLoc,
           (int) pMpApicTable->mpApicItSize);

    /* bus routing table */

    bcopy ((char *) staticMpApicBusTable,
           (char *) pMpApicTable->mpApicBtLoc,
           (int) pMpApicTable->mpApicBtSize);

    /* loApic Id translation */

    bcopy ((char *) staticMploApicIndexTable,
           (char *) pMpApicTable->mpApicLaLoc,
           (int) pMpApicTable->mpApicLaSize);

    /* logical io apic id to address mapping */

    bcopy ((char *) staticMpApicAddrTable,
           (char *) pMpApicTable->mpApicAtLoc,
           (int) pMpApicTable->mpApicAtSize);

    /* recorded id mapping */

    bcopy ((char *) staticMpApicLogicalTable,
           (char *) pMpApicTable->mpApicLtLoc,
           (int) pMpApicTable->mpApicLtSize);

    /* interrupt routing table */

    bcopy ((char *) staticMpApicInterruptTable,
           (char *) pMpApicTable->mpApicItLoc,
           (int) pMpApicTable->mpApicItSize);

    /* bus routing table */

    bcopy ((char *) staticMpApicBusTable,
           (char *) pMpApicTable->mpApicBtLoc,
           (int) pMpApicTable->mpApicBtSize);

    /* loApic Id translation */

    bcopy ((char *) staticMploApicIndexTable,
           (char *) pMpApicTable->mpApicLaLoc,
           (int) pMpApicTable->mpApicLaSize);

    intCpuUnlock(oldLevel);         /* UNLOCK INTERRUPTS */
    }
#endif  /* defined (INCLUDE_VIRTUAL_WIRE_MODE) || */
        /* defined (INCLUDE_SYMMETRIC_IO_MODE)    */

/*******************************************************************************
*
* sysHardReset - Perform hard CPU reset
*
* Performs as hard CPU reset (cold boot).
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void sysHardReset(void)
    {
    /*
     * SCH reset.Because not all board response SLPMODE low, SLPRDY# low, and
     * RSTRDY#. Using SCH WARM to reset CPU and all peripherals.
     */

    sysOutByte(RST_CNT_REG, RST_CNT_SYS_RST);

    /* ICH Full reset */

    sysOutByte(RST_CNT_REG, RST_CNT_SYS_RST | RST_CNT_CPU_RST | RST_CNT_FULL_RST);
    }

#ifdef PORT80_LED_DEBUG
/*******************************************************************************
 *
 * sysLedOut - Output to I/O port 0x80,0x81                                                         
 *
 * byte or word value output to I/O port 0x80,0x81
 *
 * RETURNS: N/A
 *
 * \NOMANUAL
 *
 */ 

void sysLedOut(short val)
    {
    sysOutWord(0x80, val);
    }
#endif /* PORT80_LED_DEBUG */

#ifdef INCLUDE_EARLY_PRINT
/*******************************************************************************
*
* earlyPrint - early serial output service
*
* This routine provides a crude method to output serial characters to
* the console port before printf services are available.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void earlyPrint
    (
    char * outString
    )
    {
#define PRINT_DELAY (115200/CONSOLE_BAUD_RATE * 100)
#define UART_BASE 0x3f8
#define UART_LCR 3
#define UART_LSR 5
    int i;

    /* set baud rate */

    sysOutByte(UART_BASE + UART_LCR, 0x80);     /* set LDR on */
    sysOutWord(UART_BASE, 0x1c200/CONSOLE_BAUD_RATE);   /* set baud rate */
    sysOutByte(UART_BASE + UART_LCR, 0x3);  /* set LDR off, 8-bit, no parity */

    if(outString)
        {
        for(i=0; i<strlen(outString); i++)
            {
            if (outString[i] == '\n')
                {
                sysOutByte(UART_BASE, '\r');
                sysUsDelay(PRINT_DELAY);
                }

            sysOutByte(UART_BASE, outString[i]);

            sysUsDelay(PRINT_DELAY);
            }
        }
    }
#endif /* INCLUDE_EARLY_PRINT */

#ifdef INCLUDE_CPU_PWR_ARCH
#define MSR_APERF   0xE8
#define MSR_MPERF   0xE7
/*******************************************************************************
*
* sysPerformanceShow - Displays the current system performance setting.
*
* This function reads the APERF and MPERF counters and determnes how many ticks
* expire in one second.  The clock rate of the APERF clock is
* tied to the CPU clock rate whereas the MPERF is fixed.  It prints out the
* elapsed time in ticks and percent CPU utilization  as an average over the
* measured second.
*
* This function is provided for test purposes only.
*
* RETURNS: N/A
*
* \NOMANUAL
*/

void sysPerformanceShow (void)
    {
    LL_UNION        aperfValue1, aperfValue2;
    LL_UNION        mperfValue1, mperfValue2;
    LL_UNION        regValue;
    long long int   aperfElapsed, mperfElapsed;
    double pctPerformance;

    taskDelay (1);  /* synchronize to clock tick */
    pentiumMsrGet (MSR_APERF, &aperfValue1.i64);
    pentiumMsrGet (MSR_MPERF, &mperfValue1.i64);
    taskDelay (sysClkRateGet ()); /* wait one second */
    pentiumMsrGet (MSR_APERF, &aperfValue2.i64);
    pentiumMsrGet (MSR_MPERF, &mperfValue2.i64);

    aperfElapsed = aperfValue2.i64 - aperfValue1.i64;

    printf ("APERF clock frequency = %lld\n", aperfElapsed);

    mperfElapsed = mperfValue2.i64 - mperfValue1.i64;
    printf ("MPERF clock frequency = %lld\n", mperfElapsed);

    pctPerformance = ((aperfElapsed - mperfElapsed) * 100)/mperfElapsed + 100.0;
    printf ("system performance rate = %2.2f pct\n", pctPerformance);

    /* read the performance register status value */

    pentiumMsrGet (0x0198, &regValue.i64);
    printf("current performance status register = 0x%llx\n", regValue.i64);
    }

/*******************************************************************************
*
* sysPerformanceGet - Compute the current system performance setting.
*
* This function reads the APERF and MPERF counters and determnes how many ticks
* expire in one second.  The clock rate of the APERF clock is
* tied to the CPU clock rate whereas the MPERF is fixed.  It returns the
* percent CPU utilization as an average over the measured second.
*
* This function is provided for test purposes only.
*
* RETURNS: percent utilization
*
* \NOMANUAL
*/

UINT32 sysPerformanceGet (void)
    {
    LL_UNION        aperfValue1, aperfValue2;
    LL_UNION        mperfValue1, mperfValue2;
    volatile void (* cpuPwrIdleEnterFunc) (void);
    long long int   aperfElapsed, mperfElapsed;
    UINT32 pctPerformance;

    taskDelay (1);  /* synchronize to clock tick */
    pentiumMsrGet (MSR_APERF, &aperfValue1.i64);
    pentiumMsrGet (MSR_MPERF, &mperfValue1.i64);
    taskDelay (sysClkRateGet ()); /* wait one second */
    pentiumMsrGet (MSR_APERF, &aperfValue2.i64);
    pentiumMsrGet (MSR_MPERF, &mperfValue2.i64);

    aperfElapsed = aperfValue2.i64 - aperfValue1.i64;

    mperfElapsed = mperfValue2.i64 - mperfValue1.i64;

    pctPerformance = ((aperfElapsed - mperfElapsed) * 100)/mperfElapsed + 100;

    return (pctPerformance);
    }

#endif /* INCLUDE_CPU_PWR_ARCH */
