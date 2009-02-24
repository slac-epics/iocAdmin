#ifndef PLX9080_EEPROM_H
#define PLX9080_EEPROM_H
/* $Id: plx9080_eeprom.h,v 1.1 2007/02/09 01:39:03 saa Exp $ */

/* PLX9080 serial EEPROM access (93CS46 device) */
#include <stdio.h>
#include <rtems.h>
#include <bsp/pci.h>

/* Primitive Driver for Accessing EEPROM attached to
 * PLX9080 or PLX9030 chips.
 *
 * NOTE: The driver is not thread-safe. Some parameters
 *       are kept in global variables that are initialized
 *       by plx9080_ee_init() -- all read/write operations
 *       affect the device addressed by 'init'.
 */

#ifndef PCI_DEVICE_ID_PLX_9030
#define PCI_DEVICE_ID_PLX_9030 0x9030
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 'Attach' to EEPROM of a PLX chip.
 *
 * 'instance': instance number (counting starts with 0)
 *             of PLX chip. E.g., '2' is the third device
 *             found when scanning the PCI bus.
 *  'plxType': what device to look for. May be 9080
 *             or 9030.
 *   'eeType': what type of EEPROM is attached to the PLX.
 *             There's no way we can figure it out - you
 *             have to look at your hardware and identify
 *             the small chip (usually close to the PLX).
 *             May be one of 46, 56, 66 identifying one
 *             of the currently supported 'ATMEL' chips
 *             93C46, 93C56 or 93C66.
 *
 *    RETURNS: zero on success, nonzero on error (device
 *             not found, illegal arguments).
 *
 *             Information about the detection process
 *             is printed to stdout.
 */
long
plx9080_ee_init(int instance, int plxType, int eeType);

/* Read EEPROM and print to FILE or stdout.
 *
 *  'address': Address of word to be read.
 *             Address may be -1 in which case the
 *             entire contents are dumped.
 *
 *             NOTE: 'address' is *not* a byte
 *             address but counts (usually 16-bit)
 *             words (width of EEPROM). I.e.,
 *             plx9080_ee_read(6, 0) reads bytes
 *             12 and 13 (word 6).
 *
 *
 *        'f': FILE pointer or NULL (-> stdout is
 *             used if user ask for all contents,
 *             nothing is printed for single-word
 *             read).
 *
 *    RETURNS: Last word read or value < 0 on error.
 */
long
plx9080_ee_read(long address, FILE *f);

/* Write word to EEPROM.
 *
 *  'address': Address of word to be written.
 *
 *             NOTE: 'address' is *not* a byte
 *             address but counts (usually 16-bit)
 *             words (width of EEPROM). I.e.,
 *             plx9080_ee_write(6, 0) clears bytes
 *             12 and 13 (word 6).
 *
 *    RETURNS: Word read back from device or
 *             value < 0 on error.
 */
long
plx9080_ee_write(int address, int value);

/* Write data from a file to EEPROM.
 *
 * 'filename': Data file to be copied into
 *             EEPROM.
 *             Format are lines of ASCII,
 *             one per data word. Data words
 *             are integer numbers, scanned
 *             with C "%i" format (i.e., '0'
 *             and '0x' prefixes can be used
 *             to indicate number base).
 *
 *    RETURNS: 0 on success, nonzero on error.
 */
long
plx9080_ee_write_from_file(char *filename);

/* Reload EEPROM data into the PLX chip
 *
 * RETURNS: 0;
 */
long
plx9080_ee_reload();

#ifdef __cplusplus
}
#endif

#endif
