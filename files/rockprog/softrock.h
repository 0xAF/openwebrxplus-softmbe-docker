
#ifndef __SOFTROCK_H
#define __SOFTROCK_H

#include <stdbool.h>

    /* Bibliotheken */
#include <libusb-1.0/libusb.h>      /* libusb 1.0 */


#define  _11_5(double_freq) (uint32_t)(double_freq*(1<<5))
#define  _8_24(double_freq) (uint32_t)(double_freq*(1<<24))
#define _11_21(double_freq) (uint32_t)(double_freq*(1<<21))
#define _43_21(double_freq) (uint64_t)(double_freq*(1<<21))


bool softrock_get_current_frequency (struct libusb_device_handle *sdr, double *freq);
bool softrock_get_set_abpf (struct libusb_device_handle *sdr, uint32_t index, uint32_t value);
bool softrock_have_abpf (void);
void softrock_show_abpf (void);
bool softrock_read_vco (struct libusb_device_handle *sdr, double *freq);
bool softrock_write_vco (struct libusb_device_handle *sdr, double freq);
bool softrock_read_xtal (struct libusb_device_handle *sdr, double *freq);
bool softrock_write_xtal (struct libusb_device_handle *sdr, double freq);
bool softrock_read_3rd (struct libusb_device_handle *sdr, double *freq);
bool softrock_write_3rd (struct libusb_device_handle *sdr, double freq);
bool softrock_read_5th (struct libusb_device_handle *sdr, double *freq);
bool softrock_write_5th (struct libusb_device_handle *sdr, double freq);
bool softrock_read_presel_mode (struct libusb_device_handle *sdr, uint32_t *mode);
bool softrock_write_presel_mode (struct libusb_device_handle *sdr, uint32_t mode);
bool softrock_read_presel_entry (struct libusb_device_handle *sdr,
                                 int index, double *freq1, double *freq2, uint32_t *pattern);
bool softrock_write_presel_entry (struct libusb_device_handle *sdr,
                                 int index, double freq1, double freq2, uint32_t pattern);
bool softrock_read_i2c (struct libusb_device_handle *sdr, uint8_t *address);
bool softrock_read_registers (struct libusb_device_handle *sdr, uint8_t value[6]);
bool softrock_read_virtual_registers (struct libusb_device_handle *sdr, uint8_t value[6]);
bool softrock_write_virtual_registers (struct libusb_device_handle *sdr, uint8_t value[6]);
bool softrock_read_virtual_vco_factor (struct libusb_device_handle *sdr, uint32_t *factor);
bool softrock_write_virtual_vco_factor (struct libusb_device_handle *sdr, long factor);
bool softrock_read_startup (struct libusb_device_handle *sdr, double *freq);
bool softrock_write_startup (struct libusb_device_handle *sdr, double freq);
bool softrock_read_smoothtune (struct libusb_device_handle *sdr, uint16_t *smoothtune);
bool softrock_write_smoothtune (struct libusb_device_handle *sdr, uint16_t smoothtune);
bool softrock_read_factory_default_registers (struct libusb_device_handle *sdr, uint8_t value[6]);
bool softrock_read_version_number (struct libusb_device_handle *sdr, uint32_t *svn);
bool softrock_read_version_string (struct libusb_device_handle *sdr, char *version, uint32_t length);
bool softrock_read_debuginfo (struct libusb_device_handle *sdr, uint8_t *debuginfo, uint32_t length);
bool softrock_write_volume (struct libusb_device_handle *sdr, int16_t volume);
bool softrock_read_demodulator_mode (struct libusb_device_handle *sdr, uint8_t *mode);
bool softrock_write_demodulator_mode (struct libusb_device_handle *sdr, uint8_t mode);
bool softrock_read_subtract_multiply (struct libusb_device_handle *sdr, int32_t *subtract1121, uint32_t *multiply1121);
bool softrock_write_subtract_multiply (struct libusb_device_handle *sdr, int32_t subtract1121, uint32_t multiply1121);
bool softrock_read_bandwidth (struct libusb_device_handle *sdr, uint32_t *bandwidth);
bool softrock_write_bandwidth (struct libusb_device_handle *sdr, uint32_t bandwidth);

#endif
