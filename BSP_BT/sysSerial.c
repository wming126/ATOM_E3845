/* sysSerial.c - BSP serial device initialization */

/*
 * Copyright 2010, 2012 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01b,01dec12,yjw  Fix build error when remove DRV_SIO_NS16550.
                 (WIND00380360)
01a,30sep10,rbc  initial creation based on itl_nehalem version 01u.
*/

/*
DESCRIPTION

This library contains routines for PC386/486 BSP serial device initialization
*/

#include "vxWorks.h"
#include <vsbConfig.h>
#include "iv.h"
#include "intLib.h"
#include "config.h"
#include "sysLib.h"
#include "drv/sio/i8250Sio.h"

/* typedefs */

typedef struct
    {
    USHORT vector;
    ULONG  baseAdrs;
    USHORT regSpace;
    USHORT intLevel;
    } I8250_CHAN_PARAS;


/* includes */

/* defines */

#define UART_REG(reg,chan) \
    (devParas[chan].baseAdrs + reg*devParas[chan].regSpace)


/* locals */

static I8250_CHAN  i8250Chan[N_UART_CHANNELS];

static I8250_CHAN_PARAS devParas[] =
    {
      {INT_NUM_COM1,COM1_BASE_ADR,UART_REG_ADDR_INTERVAL,COM1_INT_LVL},
      {INT_NUM_COM2,COM2_BASE_ADR,UART_REG_ADDR_INTERVAL,COM2_INT_LVL}
    };

/******************************************************************************
*
* sysSerialHwInit - initialize the BSP serial devices to a quiescent state
*
* This routine initializes the BSP serial device descriptors and puts the
* devices in a quiescent state.  It is called from sysHwInit() with
* interrupts locked.
*
* RETURNS: N/A
*
* SEE ALSO: sysHwInit()
*/


void sysSerialHwInit (void)
    {
    int i;

    for (i = 0; i < N_UART_CHANNELS; i++)
        {
    i8250Chan[i].int_vec = devParas[i].vector;
    i8250Chan[i].channelMode = 0;
    i8250Chan[i].lcr =  UART_REG(UART_LCR,i);
    i8250Chan[i].data =  UART_REG(UART_RDR,i);
    i8250Chan[i].brdl = UART_REG(UART_BRDL,i);
    i8250Chan[i].brdh = UART_REG(UART_BRDH,i);
    i8250Chan[i].ier =  UART_REG(UART_IER,i);
    i8250Chan[i].iid =  UART_REG(UART_IID,i);
    i8250Chan[i].mdc =  UART_REG(UART_MDC,i);
    i8250Chan[i].lst =  UART_REG(UART_LST,i);
    i8250Chan[i].msr =  UART_REG(UART_MSR,i);

    i8250Chan[i].outByte = sysOutByte;
    i8250Chan[i].inByte  = sysInByte;

#ifdef _WRS_CONFIG_SMP
    if (vxCpuIndexGet() == 0)
#else
    if (sysBp)
#endif /* _WRS_CONFIG_SMP */
        i8250HrdInit(&i8250Chan[i]);
        }

    }
/******************************************************************************
*
* sysSerialHwInit2 - connect BSP serial device interrupts
*
* This routine connects the BSP serial device interrupts.  It is called from
* sysHwInit2().
*
* Serial device interrupts cannot be connected in sysSerialHwInit() because
* the kernel memory allocator is not initialized at that point, and
* intConnect() calls malloc().
*
* RETURNS: N/A
*
* SEE ALSO: sysHwInit2()
*/

void sysSerialHwInit2 (void)
    {
    int i;

    /* connect serial interrupts */

    for (i = 0; i < N_UART_CHANNELS; i++)
        if (i8250Chan[i].int_vec)
        {
            (void) intConnect (INUM_TO_IVEC (i8250Chan[i].int_vec),
                                i8250Int, (int)&i8250Chan[i] );
#ifdef _WRS_CONFIG_SMP
        if (vxCpuIndexGet() == 0)
#else
        if (sysBp)
#endif /* _WRS_CONFIG_SMP */
                sysIntEnablePIC (devParas[i].intLevel);
            }

    }


/******************************************************************************
*
* sysSerialChanGet - get the SIO_CHAN device associated with a serial channel
*
* This routine gets the SIO_CHAN device associated with a specified serial
* channel.
*
* RETURNS: A pointer to the SIO_CHAN structure for the channel, or ERROR
* if the channel is invalid.
*/

#if defined (INCLUDE_VXBUS)
SIO_CHAN * bspSerialChanGet
#else
SIO_CHAN * sysSerialChanGet
#endif /* INCLUDE_VXBUS */
    (
    int channel     /* serial channel */
    )
    {
#if (!defined (INCLUDE_VXBUS) || defined (DRV_SIO_NS16550))
    if ((channel >= 0) && (channel < N_UART_CHANNELS))
        {
        return ((SIO_CHAN * ) &i8250Chan[channel]);
        }
#endif

    return ((SIO_CHAN *) ERROR);
    }
