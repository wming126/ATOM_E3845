/* multiboot.c -  multiboot information structure library */

/*
 * Copyright 2010-2012 (c) Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
 * modification history
 * --------------------
 *  01c,24nov12,yjw  WIND00380360 - Support get E820 information 
 *                                  from Grub MBinfo
 *  01b,24jan11,scm  WIND00252173 - adjust comment for multiboot...
 *  01a,15jan10,sem  initial creation based on itl_hanlanCreek version 01a
 */

/* An initial attempt at understand the contents of the multiboot
   information structure passed to the operating system by
   the multiboot loader.  mem and mmap fields might be useful
   in satisfying sysPhysMemTop
*/

#include <vxWorks.h>
#include <config.h>
#include <../../h/hwif/fw/bios/vxBiosE820Lib.h>
#include <../../target/src/hwif/fw/bios/vxBiosE820LibSP.h>

/* multiboot defined states */
#define MMAR_DESC_TYPE_AVAILABLE    1 /* available to OS */
#define MMAR_DESC_TYPE_RESERVED     2 /* not available */
#define MMAR_DESC_TYPE_ACPI_RECLAIM 3 /* usable by OS after reading ACPI */
#define MMAR_DESC_TYPE_ACPI_NVS     4 /* required to save between NVS sessions */

typedef struct mb_info
    {
    unsigned int  flags;
    unsigned int  mem_lower;
    unsigned int  mem_upper;
    unsigned int  boot_device;
    unsigned int  cmdline;
    unsigned int  mods_count;
    unsigned int  mods_addr;
    unsigned int  syms[4];
    unsigned int  mmap_length;
    unsigned int  mmap_addr;
    unsigned int  drives_length;
    unsigned int  drives_addr;
    unsigned int  config_table;
    unsigned int  boot_loader_name;
    unsigned int  apm_table;
    unsigned int  vbe_control_info;
    unsigned int  vbe_mode_info;
    unsigned int  vbe_mode;
    unsigned int  vbe_interface_seg;
    unsigned int  vbe_interface_off;
    unsigned int  vbe_interface_len;
    } MB_INFO;

typedef struct mmap_buf
    {
    unsigned int        size;
    unsigned long long  base_addr;
    unsigned long long  length;
    unsigned int        type;
    } __attribute__((packed)) MMAP_BUF;

#ifdef _WRS_CONFIG_LP64
const MB_INFO* pMbInfo = (void*)0xFFFFFFFFFFFFFFFFull;
#else
const MB_INFO* pMbInfo = (void*)0xFFFFFFFF;
#endif /* _WRS_CONFIG_LP64 */

void multibootInit ()
    {
    ULONG * p = (ULONG *)(MULTIBOOT_SCRATCH);

    if (*p != MULTIBOOT_BOOTLOADER_MAGIC)
        return;

    p = (ULONG *)(MULTIBOOT_SCRATCH + 8);

    pMbInfo = (MB_INFO*)*p;

    if (pMbInfo->flags & 4)
        {
#ifdef _WRS_CONFIG_LP64
        unsigned char * bootline = (unsigned char *)((ULONG)pMbInfo->cmdline);
        char * str1 = "sysbootline:\0";
        int i,j,k;

        for (i=0; bootline[i] != '\0'; i++) {
           for (j=i, k=0; str1[k] != '\0' && bootline[j]==str1[k]; j++, k++)
              ;
           if (k > 0 && str1[k] == '\0')
	   {
             bootline = (char *)&bootline[i];
	     break;
	   }
	   else
	   {
             bootline = NULL;
	     break;
	   }
        }
#else
        char * bootline;
        bootline = strstr((char*)pMbInfo->cmdline, "sysbootline:");
#endif
        if (NULL != bootline)
            strncpy (sysBootLine, bootline + strlen("sysbootline:"), 254);
        }

    /* copy memory information of Grub MBInfo to vxworks E820 info area */

#ifdef INCLUDE_BIOS_E820_MEM_AUTOSIZE
    if (pMbInfo->flags & 0x40)
           {
             MMAP_BUF * mmap;
             BIOS_E820_MAP_DESC * desc;
             BIOS_E820_TBL_INFO * info;
             unsigned int         entries;

             info = (BIOS_E820_TBL_INFO *)BIOS_E820_INFO_START;
             desc = (BIOS_E820_MAP_DESC *)BIOS_E820_DATA_START;

             info->err = BIOS_E820_MAP_SIG_ERR;
             entries = 0;

             for (mmap = (MMAP_BUF *) pMbInfo->mmap_addr;
                  (unsigned long) mmap < pMbInfo->mmap_addr + pMbInfo->mmap_length;
                  mmap = (MMAP_BUF *) ((unsigned long) mmap
                                           + mmap->size + sizeof (mmap->size)))
                  {
                  desc->type = mmap->type;
                  desc->addr = mmap->base_addr;
                  desc->len  = mmap->length;
                  desc++;
                  entries++;
                  }

             if (entries != 0)
                 {
            	 info->err  = BIOS_E820_MAP_OK;
                 info->smap = BIOS_E820_SMAP;
                 info->entries = entries;
                 info->adr = BIOS_E820_DATA_OFFSET + (entries - 1) * sizeof(BIOS_E820_MAP_DESC);
                 }
           }
#endif

    }

#undef  MULTIBOOT_DEBUG
#ifdef  MULTIBOOT_DEBUG
#include <stdio.h>
void multibootShow ()
    {
    unsigned int flags;

    ULONG * p = (ULONG *)(MULTIBOOT_SCRATCH);

    if (*p != MULTIBOOT_BOOTLOADER_MAGIC)
      return;

    p = (ULONG *)(MULTIBOOT_SCRATCH + 8);

    pMbInfo = (MB_INFO*)*p;

    flags = (unsigned int)pMbInfo->flags;
    printf ("flags: 0x%x\n", flags);

    if (flags & 1)
        printf ("mem_lower: 0x%x mem_upper: 0x%x\n",
                (unsigned int)pMbInfo->mem_lower, (unsigned int)pMbInfo->mem_upper);

    if (flags & 2)
        printf ("boot_device: %p\n", pMbInfo->boot_device);

    if (flags & 4)
        printf ("cmdline: %p\n", pMbInfo->cmdline);

    if (flags & 8)
        {
        printf ("mods_length: 0x%x\n", pMbInfo->mods_count);
        printf ("mods_addr: 0x%x\n", pMbInfo->mods_addr);
        }

    if (flags & 0x40)
        {
        int i;
        char* mmap_area;
        printf ("mmap_length: 0x%x\n", pMbInfo->mmap_length);
        printf ("mmap_addr: 0x%x\n", pMbInfo->mmap_addr);
           printf ("mmap_areas type   start address                 region length\n");

        mmap_area = (char*) ((ULONG)pMbInfo->mmap_addr);
        for (i = 0; i < pMbInfo->mmap_length;)
            {
            MMAP_BUF* mmap = (MMAP_BUF*)mmap_area;
            printf("region      %d    0x%016llx         0x%016llx\n",
                   mmap->type,
                   mmap->base_addr,
                   mmap->length);
            i += mmap->size + sizeof (unsigned int);
            mmap_area += (mmap->size + sizeof (unsigned int));
            }
        }

    if (flags & 0x80)
        {
        printf ("drives_length: 0x%x\n", pMbInfo->drives_length);
        printf ("drives_addr: 0x%x\n", pMbInfo->drives_addr);
        }
    }
#endif /* MULTIBOOT_DEBUG */
