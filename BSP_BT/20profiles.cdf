/* 20profiles.cdf - BSP profile adjustments */

/*
 * Copyright (c) 2010 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
01a,06jul10 sem  written
*/

Profile PROFILE_BOOTAPP {
    COMPONENTS +=        \
                INCLUDE_MPTABLE_BOOT_OP
}

Profile PROFILE_BOOTAPP_BASIC {
    COMPONENTS +=        \
                INCLUDE_MPTABLE_BOOT_OP
}

Profile PROFILE_BOOTROM {
    COMPONENTS +=        \
                INCLUDE_MPTABLE_BOOT_OP
}

Profile PROFILE_BOOTROM_BASIC {
    COMPONENTS +=        \
                INCLUDE_MPTABLE_BOOT_OP
}

Profile PROFILE_BOOTROM_COMPRESSED {
    COMPONENTS +=        \
                INCLUDE_MPTABLE_BOOT_OP
}

Profile PROFILE_BOOTROM_COMPRESSED_BASIC {
    COMPONENTS +=        \
                INCLUDE_MPTABLE_BOOT_OP
}
