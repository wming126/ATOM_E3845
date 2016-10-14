/* vxbPciSdhcCtrl.c - PCI SDHC host controller driver */

/*
 * Copyright (c) 2012 - 2014 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01d,04nov14.myt  add support for Bay Trail (VXW6-80833)
01c,20nov13,e_d  remove APIs wrongly published. (WIND00444661)
01b,28oct13,e_d  fix some prevent issue. (WIND00440964)
01a,28aug12,e_d  written.
*/

/*
DESCRIPTION

This is the vxbus compliant PCI Secure Digital Host Controller (SDHC)
driver which implements the functionality specified in
"SD Specifications Part A2 SD Host Controller Simplified Specification
Version 2.00"

The SDHC provides an interface between the PCI host controller and
SD/MMC memory cards.

The driver implements all the vxbus driver specific initialization routines like
pciSdhcInstInit(), pciSdhcInstInit2() and pciSdhcInstConnect().

EXTERNAL INTERFACE

The driver provides the standard vxbus external interface pciSdhcRegister().
This function registers the driver with the vxbus subsystem, and instances will
be created as needed.  If SDHC is a local bus device, each device instance
must be specified in the hwconf.c file in a BSP. If it is a Intel PCI bus device,
please ignore the following steps, only define DRV_STORAGE_SDHC in config.h or
add this component into project build is enough.

The hwconf entry can specify the following parameters:

\is

\i <clkFreq>
Specifies the clock source frequency of SDHC module. The clock source
frequency is platform dependent. Usually it is (CCB clock) / 2 on
85xx/QorIQ SOCs and (CPU clock) / 4 on Cavium Networks CNS3XXX SOC.

\i <dmaMode>
Specifies the DMA mode of SDHC. Both SDMA and PIO mode are supported now.
If this property is not explicitly specified, the driver uses SDMA by default.

\i <polling>
Specifies whether the driver uses polling mode or not. If this property is
not explicitly specified, the driver uses interrupt by default.

\i <cardWpCheck>
Specifies the card write prote status. If the board has one especial pin to
define write prote status, BSP can input the function pointer and replace old one.

\i <cardDetect>
Specifies the card insert status. If the board has one especial pin to
define card insert status, BSP can input the function pointer and replace old one.

\i <vddSetup>
Specifies the card vdd setup function. By default, the driver need not setup vdd
status. If the board has one especial function to setup it, BSP can input the
function pointer and replace old one.

\i <flags>
Specifies various features to each SOC. Currently, the following flags are
supported:

    SDHC_PIO_NEED_DELAY        :      specify whether every PIO operation needs a
                                      delay. This is found to be true on some
                                      version of many Freescale SOCs.
    SDHC_FIFO_ENDIANESS_REVERSE:      specify whether the SDHC host controller needs
                                      swap handle operation.
    SDHC_HW_SNOOP              :      specify whether the SDHC host controller has
                                      hardware snoop features. Cache operations are
                                      not performd when this flag is set.
    SDHC_MISS_CARD_INS_INT_WHEN_INIT: specify whether the SDHC host controller has
                                      card insert interrupt occured when power up with
\ie

An example hwconf entry is shown below:

If it's Freescale eSDHC controllers,
\cs
struct hcfResource pciSdhcResources[] =  {
    { "regBase",        HCF_RES_INT,    { (void *)(CCSBAR + 0x114000) } },
    { "clkFreq",        HCF_RES_ADDR,   { (void *)sysPlbClkFreqGet } },
    { "dmaMode",        HCF_RES_INT,    { (void *)0 } },
    { "polling",        HCF_RES_INT,    { (void *)0 } },
    { "flags" ,         HCF_RES_INT,    { (void *)(SDHC_PIO_NEED_DELAY | SDHC_HW_SNOOP |
                                                  SDHC_MISS_CARD_INS_INT_WHEN_INIT |
                                                  SDHC_FIFO_ENDIANESS_REVERSE) } },
};

SEE ALSO: vxBus
\tb "Intel ICH manual"
\tb "SD Specifications Part A1 Physical Layer Simplified Specification Version 2.00"
\tb "SD Specifications Part A2 SD Host Controller Simplified Specification Version 2.00"
*/

/* includes */

#include <vxWorks.h>
#include <stdio.h>
#include <semLib.h>
#include <sysLib.h>
#include <taskLib.h>
#include <vxBusLib.h>
#include <cacheLib.h>
#include <hwif/vxbus/vxBus.h>
#include <hwif/vxbus/hwConf.h>
#include <hwif/util/vxbParamSys.h>

#include <hwif/vxbus/vxbPciLib.h>
#include <hwif/vxbus/hwConf.h>
#include <drv/pci/pciConfigLib.h>
#include <drv/pci/pciIntLib.h>

#include <hwif/vxbus/vxbSdLib.h>
#include <../src/hwif/h/vxbus/vxbAccess.h>
#include <../src/hwif/h/sd/vxbSdhcCtrl.h>

/* locals */

IMPORT void vxbUsDelay (int	delayTime);
IMPORT void vxbMsDelay (int	delayTime);

LOCAL void pciSdhcInstInit(VXB_DEVICE_ID pInst);
LOCAL void pciSdhcInstInit2 (VXB_DEVICE_ID pInst);
LOCAL void pciSdhcInstConnect (VXB_DEVICE_ID pInst);
LOCAL void pciSdhcCtrlMonitor(VXB_DEVICE_ID pDev);
LOCAL void pciSdhcVddSetup (VXB_DEVICE_ID pDev, UINT32 vdd);
LOCAL STATUS pciSdhcDevInit(VXB_DEVICE_ID pDev);
LOCAL STATUS pciSdhcCmdIssue (VXB_DEVICE_ID pDev, SD_CMD * pSdCmd);
LOCAL STATUS pciSdhcSpecInfoGet (VXB_DEVICE_ID pDev,
                                 void ** pHostSpec,
                                 VXB_SD_CMDISSUE_FUNC * pCmdIssue);

LOCAL PCI_DEVVEND pciSdhcIdList[] =
{
    {TOPCLIFF0_DEVICE_ID,   					INTEL_VENDOR_ID},
    {TOPCLIFF1_DEVICE_ID,   					INTEL_VENDOR_ID},
    {PCI_DEVICE_ID_INTEL_BAY_TRAIL_EMMC_441,   	INTEL_VENDOR_ID},
    {PCI_DEVICE_ID_INTEL_BAY_TRAIL_SD,   		INTEL_VENDOR_ID}

};

LOCAL DRIVER_INITIALIZATION pciSdhcFuncs =
    {
    pciSdhcInstInit,        /* devInstanceInit */
    pciSdhcInstInit2,       /* devInstanceInit2 */
    pciSdhcInstConnect      /* devConnect */
    };

LOCAL device_method_t vxbPciSdhcCtrl_methods[] =
    {
    DEVMETHOD (vxbSdSpecInfoGet, pciSdhcSpecInfoGet),
    DEVMETHOD (sdBusCtlrInterruptInfo, sdhcInterruptInfo),
    DEVMETHOD(busCtlrDevCtlr, sdhcDevControl),
    DEVMETHOD_END
    };

LOCAL PCI_DRIVER_REGISTRATION pciSdhcRegistration =
{
    {
    NULL,
    VXB_DEVID_DEVICE,
    VXB_BUSID_PCI,
    VXB_VER_5_0_0,
    INTEL_SDHC_NAME,
    &pciSdhcFuncs,
    vxbPciSdhcCtrl_methods,
    NULL,
    },
    NELEMENTS(pciSdhcIdList),
    &pciSdhcIdList[0],
};

IMPORT UCHAR erfLibInitialized;

/*******************************************************************************
*
* pciSdhcRegister - register PCI SDHC driver
*
* This routine registers the freescale SDHC driver with the vxbus subsystem.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

void pciSdhcRegister (void)
    {
    vxbDevRegister ((struct vxbDevRegInfo *)&pciSdhcRegistration);
    }

/*******************************************************************************
*
* pciSdhcInstInit - first level initialization routine of pci SDHC device
*
* This routine performs the first level initialization of the pci SDHC device.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL void pciSdhcInstInit
    (
    VXB_DEVICE_ID pInst
    )
    {
    sdhcCtrlInstInit(pInst);
    }

/*******************************************************************************
*
* pciSdhcInstInit2 - second level initialization routine of pci SDHC device
*
* This routine performs the second level initialization of the pci SDHC device.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL void pciSdhcInstInit2
    (
    VXB_DEVICE_ID pInst
    )
    {
    SDHC_DEV_CTRL * pDrvCtrl;
    struct hcfDevice * pHcf;

    sdhcCtrlInstInit2(pInst);
    pDrvCtrl = (SDHC_DEV_CTRL *)(pInst->pDrvCtrl);
    if (pDrvCtrl == NULL)
        return;

    pDrvCtrl->flags = SDHC_HW_SNOOP;
    pDrvCtrl->sdHostCtrl.dmaMode = SDHC_DMA_MODE_SDMA;
    pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdHostCtrlInit = pciSdhcDevInit;
    pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdVddSetup = pciSdhcVddSetup;

    pHcf = (struct hcfDevice *)hcfDeviceGet (pInst);
    if (pHcf != NULL)
        {

        /* Need not check return status at here */

        (void)devResourceGet (pHcf, "dmaMode", HCF_RES_INT,
                             (void *)&(pDrvCtrl->sdHostCtrl.dmaMode));

        /*
         * resourceDesc {
         * The polling resource specifies whether
         * the driver uses polling mode or not.
         * If this property is not explicitly
         * specified, the driver uses interrupt
         * by default. }
         */

        /* Need not check return status at here */

        (void)devResourceGet (pHcf, "polling", HCF_RES_INT,
                             (void *)&(pDrvCtrl->sdHostCtrl.polling));

        /*
         * resourceDesc {
         * The flags resource specifies various
         * controll flags of the host controller. }
         */

        /* Need not check return status at here */

        (void)devResourceGet (pHcf, "flags", HCF_RES_INT,
                             (void *)&(pDrvCtrl->flags));

        /* Need not check return status at here */

        (void)devResourceGet (pHcf, "cardWpCheck", HCF_RES_ADDR,
                             (void *)
                             &(pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdCardWpCheck));

        /* Need not check return status at here */

        (void)devResourceGet (pHcf, "cardDetect", HCF_RES_ADDR,
                             (void *)
                             &(pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdCardInsertSts));

        /* Need not check return status at here */

        (void)devResourceGet (pHcf, "vddSetup", HCF_RES_ADDR,
                             (void *)
                             &(pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdVddSetup));
        }
    }

/*******************************************************************************
*
* pciSdhcInstConnect - third level initialization routine of pci SDHC device
*
* This routine performs the third level initialization of the pci SDHC device.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL void pciSdhcInstConnect
    (
    VXB_DEVICE_ID pInst
    )
    {
    UINT32 val;
    STATUS rc;
    SDHC_DEV_CTRL * pDrvCtrl;
    SD_HOST_CTRL * pHostCtrl;
    pDrvCtrl = (SDHC_DEV_CTRL *)pInst->pDrvCtrl;
    if (pDrvCtrl == NULL)
        return;

    pHostCtrl = (SD_HOST_CTRL *)pDrvCtrl;
    rc = sdhcCtrlInstConnect((SD_HOST_CTRL *)pDrvCtrl);
    if (rc == ERROR)
        return;

    pHostCtrl->sdHostSpec.vxbSdBusWidthSetup = pHostCtrl->sdHostOps.vxbSdBusWidthSetup;
    pHostCtrl->sdHostSpec.vxbSdCardWpCheck = pHostCtrl->sdHostOps.vxbSdCardWpCheck;
    pHostCtrl->sdHostSpec.vxbSdClkFreqSetup = pHostCtrl->sdHostOps.vxbSdClkFreqSetup;
    pHostCtrl->sdHostSpec.vxbSdResumeSet = pHostCtrl->sdHostOps.vxbSdResumeSet;
    pHostCtrl->sdHostSpec.vxbSdVddSetup =  pHostCtrl->sdHostOps.vxbSdVddSetup;
    pHostCtrl->sdHostSpec.capbility = pHostCtrl->capbility;
    pHostCtrl->sdHostSpec.maxTranSpeed = pHostCtrl->maxTranSpeed;

    taskSpawn ("sdBusMonitor", 100, 0,
               8192, (FUNCPTR)pciSdhcCtrlMonitor, (_Vx_usr_arg_t)pInst,
               0, 0, 0, 0, 0, 0, 0, 0, 0);

    /* setup the interrupt mask */

    pDrvCtrl->intMask = (IRQ_DATA | IRQ_CMD);
    pDrvCtrl->intMask |= IRQ_AC12E;
    pDrvCtrl->intMask |= IRQ_DINT;

    if (pDrvCtrl->sdHostCtrl.dmaMode == SDHC_DMA_MODE_PIO)
        pDrvCtrl->intMask |= (IRQ_BRR | IRQ_BWR);

    pDrvCtrl->intMask |= IRQ_CINS;

    CSR_WRITE_4 (pInst, SDHC_IRQSTATEN, pDrvCtrl->intMask);

    /* enable SDHC interrupts */

    if (pDrvCtrl->sdHostCtrl.polling == FALSE)
        {

        /* connect and enable interrupt */

        if (pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdIsr == NULL)
            return;

        rc = vxbIntConnect (pDrvCtrl->sdHostCtrl.pDev,
                           0,
                           (VOIDFUNCPTR)pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdIsr,
                           pDrvCtrl->sdHostCtrl.pDev);
        if (rc == ERROR)
            return;
        rc = vxbIntEnable (pDrvCtrl->sdHostCtrl.pDev,
                          0,
                          (VOIDFUNCPTR)pDrvCtrl->sdHostCtrl.sdHostOps.vxbSdIsr,
                          pDrvCtrl->sdHostCtrl.pDev);
        if (rc == ERROR)
            return;

        CSR_WRITE_4 (pInst, SDHC_IRQSIGEN, pDrvCtrl->intMask);
        }

    pDrvCtrl->flags |= SDHC_MISS_CARD_INS_INT_WHEN_INIT;
    if (pDrvCtrl->flags & SDHC_MISS_CARD_INS_INT_WHEN_INIT)
        {

        /* don't miss an already inserted card */

        val  = CSR_READ_4(pInst, SDHC_PRSSTAT);
        if (val & PRSSTAT_CINS)
            {
            semGive(pDrvCtrl->sdHostCtrl.devChange);
            }
        }

    return;
    }

/*******************************************************************************
*
* pciSdhcDevInit - eSDHC per device specific initialization
*
* This routine performs per device specific initialization of eSDHC.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL STATUS pciSdhcDevInit
    (
    VXB_DEVICE_ID pInst
    )
    {
    UINT32 hostCap;
    STATUS rc;
    SDHC_DEV_CTRL * pDrvCtrl;
    PCI_HARDWARE * pPciDevInfo;
    pDrvCtrl = (SDHC_DEV_CTRL *)pInst->pDrvCtrl;
    pPciDevInfo = (PCI_HARDWARE *)pInst->pBusSpecificDevInfo;

    if (pDrvCtrl == NULL || pPciDevInfo == NULL)
        return ERROR;

    if ((pPciDevInfo->pciVendId == INTEL_VENDOR_ID) && 
	    ((pPciDevInfo->pciDevId == PCI_DEVICE_ID_INTEL_BAY_TRAIL_SD) ||
		 (pPciDevInfo->pciDevId == PCI_DEVICE_ID_INTEL_BAY_TRAIL_EMMC_441)))
        {
        /* maximum supported controller speed in eMMC 4.41 compliance */
        pDrvCtrl->sdHostCtrl.maxTranSpeed =  MMC_CLK_FREQ_52MHZ;
        CSR_SETBIT_4 (pInst, SDHC_PROCTL, PROCTL_SD_PWR_EN_BTRAIL);
        vxbMsDelay(1);
        CSR_CLRBIT_4 (pInst, SDHC_PROCTL, PROCTL_SD_PWR_EN_BTRAIL);
        vxbMsDelay(1);
        } 
    else
        {
        CSR_WRITE_4 (pInst, SDHC_SRST, RESET_ON);
        CSR_WRITE_4 (pInst, SDHC_SRST, RESET_RELEASE);
        }

    rc = sdhcCtrlInit(pInst);
    if (rc == ERROR)
        return ERROR;

    hostCap = CSR_READ_4 (pInst, SDHC_HOSTCAPBLT);
    pDrvCtrl->sdHostCtrl.curClkFreq = ((hostCap >> HOSTCAPBLT_FREQ_SHIT)
                                        & HOSTCAPBLT_FREQ_MASK)
                                        * 1000000;

    return OK;
    }

/*******************************************************************************
*
* pciSdhcVddSetup - setup the SD bus voltage level and power it up
*
* This routine setups the SD bus voltage and powers it up. This
* routine will be called in the SDHC driver.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL void pciSdhcVddSetup
    (
    VXB_DEVICE_ID pDev,
    UINT32        vdd
    )
    {

    /* setup power value and turn on sd bus power */

    CSR_SETBIT_4 (pDev, SDHC_PROCTL, PROCTL_SD_PWR_EN |
                     (vdd << PROCTL_VOLT_SEL_SHIFT));

    vxbMsDelay(100);
    }

/*******************************************************************************
*
* pciSdhcCmdIssue - issue the command to be sent
*
* This routine issue the command to be sent.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

STATUS pciSdhcCmdIssue
    (
    VXB_DEVICE_ID pDev,
    SD_CMD * pSdCmd
    )
    {
    SD_HOST_CTRL * pSdHostCtrl;
    STATUS rc;

    pSdHostCtrl = (SD_HOST_CTRL *)(pDev->pDrvCtrl);
    if (pSdHostCtrl == NULL)
        return ERROR;

    if (pSdHostCtrl->polling)
        rc = sdhcCtrlCmdIssuePoll(pDev, pSdCmd);
    else
        rc = sdhcCtrlCmdIssue(pDev, pSdCmd);

    return (rc);
    }

/*******************************************************************************
*
* pciSdhcCtrlMonitor - SDHC insert status checker
*
* This function will check SDHC insert status. If target device insert status
* is TRUE, the function will run sdioBusAnnounceDevices to add target device
* to vxbus system. If FLASE, will run vxbDevRemovalAnnounce to remove the target
* device from vxbus system.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
* \NOMANUAL
*/

LOCAL void pciSdhcCtrlMonitor
    (
    VXB_DEVICE_ID pDev
    )
    {
    STATUS rc;
    int i = 0;
    VXB_DEVICE_ID pDevList;
    SD_HOST_CTRL * pSdHostCtrl;

    pSdHostCtrl = (SD_HOST_CTRL *)(pDev->pDrvCtrl);
    if (pSdHostCtrl == NULL)
        return;

    while (erfLibInitialized == FALSE)
        taskDelay (sysClkRateGet ());

    while(1)
        {
        rc = pSdHostCtrl->sdHostOps.vxbSdCardInsertSts(pDev);
        if (rc)
            {
            if (pSdHostCtrl->attached == TRUE)
                continue;
            if (pSdHostCtrl->sdHostOps.vxbSdVddSetup != NULL)
                pSdHostCtrl->sdHostOps.vxbSdVddSetup (pDev, pSdHostCtrl->vdd);
            if (pSdHostCtrl->sdHostOps.vxbSdClkFreqSetup != NULL)
                pSdHostCtrl->sdHostOps.vxbSdClkFreqSetup(pDev, SDMMC_CLK_FREQ_400KHZ);
            if (pSdHostCtrl->sdHostOps.vxbSdBusWidthSetup != NULL)
                pSdHostCtrl->sdHostOps.vxbSdBusWidthSetup (pDev, SDMMC_BUS_WIDTH_1BIT);

            /* Need not check return status at here */

            (void)sdBusAnnounceDevices(pDev, NULL);
            pSdHostCtrl->attached = TRUE;
            }
        else
            {
            pDevList = pDev->u.pSubordinateBus->instList;
            for(i = 0; i < MAX_TARGET_DEV; i++)
                {
                if (pDevList != NULL)
                    {

                    /* Need not check return status at here */

                    vxbDevRemovalAnnounce(pDevList);
                    pDevList = pDevList->pNext;
                    }
                else
                	break;
                }
            pSdHostCtrl->attached = FALSE;
            }
        taskDelay (2 * sysClkRateGet());
        }
    }

/*******************************************************************************
*
* pciSdhcSpecInfoGet - get host controller spec info
*
* This routine gets host controller spec info.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
* \NOMANUAL
*/

STATUS pciSdhcSpecInfoGet
    (
    VXB_DEVICE_ID pDev,
    void ** pHostSpec,
    VXB_SD_CMDISSUE_FUNC * pCmdIssue
    )
    {
    SD_HOST_CTRL * pHostCtrl;

    pHostCtrl = (SD_HOST_CTRL *)pDev->pDrvCtrl;
    if (pHostCtrl == NULL)
        return ERROR;

    *pHostSpec = (void *)(&(pHostCtrl->sdHostSpec));
    *pCmdIssue = pciSdhcCmdIssue;
    return (OK);
    }
