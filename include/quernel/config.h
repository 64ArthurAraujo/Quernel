#pragma once

#define QUER_INFORMATIONAL 1 /* Enable informational logging */
#define QUER_DEBUG         1 /* Enable debug level logging */
#define QUER_ERR           1 /* Enable error logging */

/* 
 * WARNING!!
 *
 * This option unlocks Quernel to detect all disks, beware that this may 
 * be dangerous and might possibly compromise your main operating system 
 * if some nefarious bug occurs (which is very likely), only disable
 * this when running on in a VM or bare-metal with only the USB device connected.
 */
#define ALLOW_MULTIDISK    1 