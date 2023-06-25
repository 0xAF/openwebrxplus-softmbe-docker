/*
 * main.c
 */

#if __linux__
#define _XOPEN_SOURCE 500
#endif

    /* Standard-Libraries */
#include <stdbool.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

    /* Bibliotheken */
#include <libusb-1.0/libusb.h>
#include <popt.h>                       /* Parsen von Kommandozeilen-Optionen */

#include "global.h"
#include "softrock.h"

struct libusb_context *usb_context;
struct libusb_device_handle *fifisdr;


/* Variablen für Argumente aus der Kommandozeile */
int cmdline_write = false;
long cmdline_vid = 0x16C0;
long cmdline_pid = 0x05DC;
char *cmdline_manufacturer_string = "www.ov-lennestadt.de";
char *cmdline_serial_string = "";
int cmdline_list_devices = false;
int cmdline_abpf = false;
long cmdline_index = -1;
double cmdline_freq = -999.0;
int cmdline_vco = false;
int cmdline_xtal = false;
int cmdline_3rd = false;
int cmdline_5th = false;
int cmdline_presel = false;
int cmdline_i2c = false;
long cmdline_pattern = -1;
long cmdline_mode = -1;
int cmdline_regs = false;
int cmdline_vregs = false;
char *cmdline_regset = "";
double cmdline_freq_from = -999.0;
double cmdline_freq_to = -999.0;
int cmdline_virtual_vco_factor = false;
long cmdline_factor = -1;
int cmdline_startup = false;
int cmdline_version = false;
const char rockprog_version[] = "06.08.2016";   // Datum als Versionsnummer
int cmdline_smoothtune = false;
long cmdline_ppm = -1;
int cmdline_autotune = false;
int cmdline_firmware = false;
int cmdline_debuginfo = false;
long cmdline_volume = -1;
int cmdline_demod = false;
char *cmdline_demodmode = "";
double cmdline_subtract = -999.0;
double cmdline_multiply = -999.0;
int cmdline_offset = false;
int cmdline_bandwidth = false;



/* USB-Fehler in Klartext */
void print_usb_error (int error)
{
    switch (error) {
    case LIBUSB_ERROR_IO:
        printf ("libusb: Input/output error\n");
    break;
    case LIBUSB_ERROR_ACCESS:
        printf ("libusb: Keine Zugriffsrechte\n");
    break;
    case LIBUSB_ERROR_NO_DEVICE:
        printf ("libusb: Jemand war schneller (FiFi-SDR wieder abgesteckt)\n");
    break;
    case LIBUSB_ERROR_NOT_FOUND:
        printf ("libusb: Entity not found (falsche Version von libusb?)\n");
    break;
    case LIBUSB_ERROR_BUSY:
        printf ("libusb: Resource busy\n");
    break;
    case LIBUSB_ERROR_TIMEOUT:
        printf ("libusb: Timeout\n");
    break;
    case LIBUSB_ERROR_PIPE:
        printf ("libusb: 'Pipe Error'\n");
    break;
    case LIBUSB_ERROR_NO_MEM:
        printf ("libusb: Kein Speicher\n");
    break;
    case LIBUSB_ERROR_NOT_SUPPORTED:
        printf ("libusb: Operation not supported or unimplemented on this platform\n");
    break;

    default: printf ("Unbekannter USB-Fehler (%d)\n", error); break;
    };
}


/* FiFi-SDR suchen und öffnen */
bool such_fifi (bool listOnly)
{
    struct libusb_device **devices;                 /* Geräteliste */
    ssize_t cnt;                                    /* Anzahl Geräte */
    ssize_t n;
    struct libusb_device *device;
    struct libusb_device_descriptor desc_device;
    struct libusb_device_handle *handle;
    int error;
    int iString;
    char device_string[100];
    bool found;
    bool stringsOk;


    /* Info über alle angeschlossenen Geräte holen */
    cnt = libusb_get_device_list (usb_context, &devices);
    if (cnt <= 1)
    {
        printf ("Kein USB-Gerät an diesem PC angeschlossen\n");
        return false;
    }

    /* Ist ein passendes Gerät dabei? */
    found = false;
    for (n = 0; n < cnt; n++) {
        /* Geräteinfo holen */
        device = devices[n];
        error = libusb_get_device_descriptor (device, &desc_device);
        if (error == 0) {
            /* VID und PID müssen in jedem Fall passen */
            if ((cmdline_vid == desc_device.idVendor) &&
                (cmdline_pid == desc_device.idProduct)) {

                /* Device öffnen um Strings lesen zu können */
                error = libusb_open(device, &handle);
                if (error == 0) {
                    stringsOk = false;

                    /* Manufacturer-String abfragen um FiFi-SDR von anderen Libusb-Geräten zu unterscheiden */
                    iString = desc_device.iManufacturer;
                    if (iString != 0) {
                        /* Manufacturer-String enthält reine ASCII-Zeichen */
                        error = libusb_get_string_descriptor_ascii(handle, iString, (unsigned char *)device_string, sizeof(device_string));
                        if (error > 0) {
                            /* Der String muß genau stimmen */
                            if (!strcmp(device_string, cmdline_manufacturer_string)) {
                                stringsOk = true;
                            }
                        }
                    }

                    /* Seriennummer lesen wenn möglich */
                    iString = desc_device.iSerialNumber;
                    if (iString == 0) {
                        if (listOnly) {
                            printf("FiFi-SDR ohne Seriennummer...\n");
                        }
                    }
                    else {
                        /* Serial-String enthält reine ASCII-Zeichen */
                        error = libusb_get_string_descriptor_ascii(handle, iString, (unsigned char *)device_string, sizeof(device_string));
                        if (error > 0) {
                            if (listOnly) {
                                printf("%s\n", device_string);
                            }
                        }
                    }

                    /* Wenn eine Seriennummer angegeben ist, dann diese zur Auswahl heranziehen */
                    if (strlen(cmdline_serial_string) > 0) {
                        if (iString == 0) {
                            /* Device hat keine Seriennummer */
                            printf("FiFi-SDR hat keine Seriennummer (iSerial = 0)\n");
                            stringsOk = false;
                        }
                        else {
                            /* Der vorgegebene String muß in der tatsächlichen Seriennummer enthalten sein */
                            stringsOk = strstr(device_string, cmdline_serial_string) ? true : false;
                        }
                    }

                    /* Device merken falls alle Angaben stimmen */
                    if (stringsOk && !listOnly) {
                        fifisdr = handle;
                        found = true;
                        break;
                    }

                    libusb_close(handle);
                }
                else {
                    print_usb_error (error);
                    printf ("libusb_open() fehlgeschlagen\n");
                }
            }
        }
    }

    libusb_free_device_list (devices, 1);

    return found;
}




#include <stdio.h>
int main (int argc, char *argv[])
{
    bool ok;
    int i;


    /* Kommandozeilen-Parameter */
    struct poptOption optionsTable[] =
    {
        { "write", 'w', POPT_ARG_NONE, &cmdline_write, 0,
                "Werte schreiben" },
        { "vid", '\0', POPT_ARG_LONG, &cmdline_vid, 0,
                "USB-PID des FiFi-SDR (Vorgabe: 0x16C0)" },
        { "manufacturer", '\0', POPT_ARG_STRING, &cmdline_manufacturer_string, 0,
                "Manufacturer-String (Vorgabe: www.ov-lennestadt.de)" },
        { "serial", '\0', POPT_ARG_STRING, &cmdline_serial_string, 0,
                "Mehrere FiFi-SDRs: (Teil-)String der Seriennummer" },
        { "list", 'l', POPT_ARG_NONE, &cmdline_list_devices, 0,
                "Zeige Liste aller angeschlossenen FiFi-SDRs" },
        { "pid", '\0', POPT_ARG_LONG, &cmdline_pid, 0,
                "USB-VID des FiFi-SDR (Vorgabe: 0x05DC)" },
        { "abpf", '\0', POPT_ARG_NONE, &cmdline_abpf, 0,
                "Zugriff auf automatisches Bandpaßfilter" },
        { "index", '\0', POPT_ARG_LONG, &cmdline_index, 0,
                "Index für z.B. ABPF" },
        { "freq", '\0', POPT_ARG_DOUBLE, &cmdline_freq, 0,
                "Frequenzangabe [MHz]" },
        { "vco", '\0', POPT_ARG_NONE, &cmdline_vco, 0,
                "VCO-Frequenz" },
        { "xtal", '\0', POPT_ARG_NONE, &cmdline_xtal, 0,
                "Quarz-Frequenz" },
        { "3rd", '\0', POPT_ARG_NONE, &cmdline_3rd, 0,
                "Start für RX auf 3. Oberwelle" },
        { "5th", '\0', POPT_ARG_NONE, &cmdline_5th, 0,
                "Start für RX auf 5. Oberwelle" },
        { "presel", '\0', POPT_ARG_NONE, &cmdline_presel, 0,
                "Preselektor" },
        { "pattern", '\0', POPT_ARG_LONG, &cmdline_pattern, 0,
                "Pattern für z.B. Preselektor" },
        { "mode", '\0', POPT_ARG_LONG, &cmdline_mode, 0,
                "Modus für Preselektor" },
        { "freq-from", '\0', POPT_ARG_DOUBLE, &cmdline_freq_from, 0,
                "Startfrequenz [MHz]" },
        { "freq-to", '\0', POPT_ARG_DOUBLE, &cmdline_freq_to, 0,
                "Endfrequenz [MHz]" },
        { "i2c", '\0', POPT_ARG_NONE, &cmdline_i2c, 0,
                "I2C-Adresse lesen" },
        { "regs", '\0', POPT_ARG_NONE, &cmdline_regs, 0,
                "Si570-Register" },
        { "vregs", '\0', POPT_ARG_NONE, &cmdline_vregs, 0,
                "Virtuelle Si570-Register" },
        { "regset", '\0', POPT_ARG_STRING, &cmdline_regset, 0,
                "Register-Werte für Si570 (6 Hex-Werte mit Komma getrennt: 0xAB,0xCD,...)" },
        { "vfact", '\0', POPT_ARG_NONE, &cmdline_virtual_vco_factor, 0,
                "Faktor für virtuellen VCO" },
        { "factor", '\0', POPT_ARG_LONG, &cmdline_factor, 0,
                "Faktor (zusammen mit --vfact)" },
        { "startup", '\0', POPT_ARG_NONE, &cmdline_startup, 0,
                "Startup-Frequenz" },
        { "version", '\0', POPT_ARG_NONE, &cmdline_version, 0,
#if __linux__
                "rockprog Version abfragen (SVN Rev)" },
#else
                "rockprog Version abfragen (SVN Rev. Linux Basisversion / SVN Rev. Win32 Port)" },
#endif
        { "smooth", '\0', POPT_ARG_NONE, &cmdline_smoothtune, 0,
                "Smooth Tune" },
        { "ppm", '\0', POPT_ARG_LONG, &cmdline_ppm, 0,
                "ppm-Wert (für --smooth)" },
        { "autotune", '\0', POPT_ARG_NONE, &cmdline_autotune, 0,
                "Quarzfrequenz automatisch abgleichen" },
        { "firmware", '\0', POPT_ARG_NONE, &cmdline_firmware, 0,
                "Version der Firmware auslesen" },
        { "debuginfo", '\0', POPT_ARG_NONE, &cmdline_debuginfo, 0,
                "Einige Debug-Werte auslesen" },
        { "volume", '\0', POPT_ARG_LONG, &cmdline_volume, 0,
                "Volume (0 oder 1)" },
        { "demod", '\0', POPT_ARG_NONE, &cmdline_demod, 0,
                "Demodulator" },
        { "demodmode", '\0', POPT_ARG_STRING, &cmdline_demodmode, 0,
                "Demodulator-Modus (LSB, USB, AM, FM, WBFM)" },
        { "offset", '\0', POPT_ARG_NONE, &cmdline_offset, 0,
                "Offset (mit --subtract und --multiply)" },
        { "subtract", '\0', POPT_ARG_DOUBLE, &cmdline_subtract, 0,
                "für --offset" },
        { "multiply", '\0', POPT_ARG_DOUBLE, &cmdline_multiply, 0,
                "für --offset" },
        { "bandwidth", '\0', POPT_ARG_NONE, &cmdline_bandwidth, 0,
                "Bandbreite Demodulator-Filter (mit --freq)" },
        POPT_AUTOHELP
        { NULL, POPT_ARG_NONE, 0, NULL, 0 }
    };

    /* Kommandozeile parsen */
    poptContext optCon;
    optCon = poptGetContext (NULL, argc, (const char **)argv, optionsTable, 0);
    poptSetOtherOptionHelp( optCon, " [options]" );

    /* Kurz-Hilfe, wenn gar keine Argumente angegeben werden. */
    if (argc < 2) {
        poptPrintUsage( optCon, stderr, 0 );
        return 1;
    }

    /* Optionskürzel abfragen */
    int rc;
    while ((rc = poptGetNextOpt(optCon)) >= 0) {
	}

    /* Prüfen, ob ein Fehler bei der Bearbeitung der Optionen aufgetreten ist. (-1 ist ok!) */
    if (rc < -1) {
        printf("\"%s\" %s\n", poptBadOption(optCon, POPT_BADOPTION_NOALIAS ), poptStrerror(rc));
        return 1;
    }

    poptFreeContext(optCon);

    if (cmdline_version)
    {
        printf( "%s\n",rockprog_version);
    }

    /* libusb initialisieren */
    if (libusb_init (&usb_context) != 0)
    {
        printf ("libusb_init ging daneben :-(\n");
        return 1;
    }

    /* FiFi-SDR suchen und öffnen */
    if (such_fifi(cmdline_list_devices))
    {
        ok = true;

        /* ABPF? */
        if (cmdline_abpf)
        {
            /* Beim Schreiben Werte prüfen */
            if (cmdline_write)
            {
                if ((cmdline_index < 0) || (cmdline_freq < 0.0))
                {
                    ok = false;
                }
            }
            else
            {
                /* Index und Frequenz haben feste Werte */
                cmdline_index = 255;
                cmdline_freq = 0.0;
            }

            if (ok)
            {
                if (softrock_get_set_abpf (fifisdr, cmdline_index, _11_5(4.0 * cmdline_freq)))
                {
                    /* Aktuelle Werte zeigen */
                    softrock_show_abpf();
                }
            }
        }

        /* VCO-Frequenz */
        if (cmdline_vco)
        {
            /* Beim Schreiben Werte prüfen */
            if (cmdline_write)
            {
                if (cmdline_freq < 0.0)
                {
                    printf ("Keine Frequenz angegeben\n");
                }
                else
                {
                    softrock_write_vco (fifisdr, cmdline_freq);
                }
            }
            else
            {
                double f;
                if (softrock_read_vco (fifisdr, &f))
                {
                    printf ("VCO = %f MHz\n", f);
                }
            }
        }

        /* Quarz-Frequenz */
        if (cmdline_xtal)
        {
            /* Beim Schreiben Werte prüfen */
            if (cmdline_write)
            {
                if (cmdline_freq < 0.0)
                {
                    printf ("Keine Frequenz angegeben\n");
                }
                else
                {
                    softrock_write_xtal (fifisdr, cmdline_freq);
                }
            }
            else
            {
                double f;
                if (softrock_read_xtal (fifisdr, &f))
                {
                    printf ("XTAL = %f MHz\n", f);
                }
            }
        }


        /* Start-Frequenz 3. Oberwelle */
        if (cmdline_3rd)
        {
            /* Beim Schreiben Werte prüfen */
            if (cmdline_write)
            {
                if (cmdline_freq < 0.0)
                {
                    printf ("Keine Frequenz angegeben\n");
                }
                else
                {
                    softrock_write_3rd (fifisdr, cmdline_freq);
                }
            }
            else
            {
                double f;
                if (softrock_read_3rd (fifisdr, &f))
                {
                    printf ("3. Oberwelle ab = %f MHz\n", f);
                }
            }
        }

        /* Start-Frequenz 5. Oberwelle */
        if (cmdline_5th)
        {
            /* Beim Schreiben Werte prüfen */
            if (cmdline_write)
            {
                if (cmdline_freq < 0.0)
                {
                    printf ("Keine Frequenz angegeben\n");
                }
                else
                {
                    softrock_write_5th (fifisdr, cmdline_freq);
                }
            }
            else
            {
                double f;
                if (softrock_read_5th (fifisdr, &f))
                {
                    printf ("5. Oberwelle ab = %f MHz\n", f);
                }
            }
        }

        /* Preselektor */
        if (cmdline_presel)
        {
            /* Beim Schreiben Werte prüfen */
            if (cmdline_write)
            {
                if (cmdline_mode != -1)
                {
                    if ((cmdline_mode >= 0) && (cmdline_mode <= 3))
                    {
                        softrock_write_presel_mode (fifisdr, cmdline_mode);
                    }
                    else
                    {
                        printf ("--mode nicht in [0,3]\n");
                    }
                }
                else
                {
                    if ((cmdline_index == -1) ||
                        (cmdline_freq_from < 0.0) ||
                        (cmdline_freq_to < 0.0) ||
                        (cmdline_pattern == -1))
                    {
                        printf ("--freq-from/--freq-to/--pattern prüfen\n");
                    }
                    else
                    {
                        softrock_write_presel_entry (fifisdr,
                                                     cmdline_index,
                                                     cmdline_freq_from,
                                                     cmdline_freq_to,
                                                     cmdline_pattern);
                    }
                }
            }
            else
            {
                uint32_t presel_mode;
                if (softrock_read_presel_mode (fifisdr, &presel_mode))
                {
                    printf ("Preselektor-Modus = %d%s\n",
                            presel_mode,
                            (presel_mode == 0) ? " (Es gelten die Umschaltpunkte für ABPF)" : "");
                }

                uint32_t pattern;
                double f1, f2;
                for (i = 0; i < 16; i++)
                {
                    if (softrock_read_presel_entry (fifisdr, i, &f1, &f2, &pattern))
                    {
                        printf ("%2d: %10f - %10f MHz, Ausgänge = %c%c%c%cb (%d)\n",
                                      i, f1, f2,
                                      pattern & 8 ? '1' : '0',
                                      pattern & 4 ? '1' : '0',
                                      pattern & 2 ? '1' : '0',
                                      pattern & 1 ? '1' : '0',
                                      pattern & 0x0F
                                      );
                    }
                }
            }
        }

        /* I2C-Adresse */
        if (cmdline_i2c)
        {
            /* Schreiben nicht unterstützt */
            if (cmdline_write)
            {
                printf ("Schreiben der I2C-Adresse nicht möglich (wird automatisch bestimmt).\n");
            }
            else
            {
                uint8_t addr;
                if (softrock_read_i2c (fifisdr, &addr))
                {
                    printf ("I2C-Adresse = 0x%02X\n", (int)addr);
                }
            }
        }

        /* Si570-Register lesen/schreiben */
        if (cmdline_regs || cmdline_vregs)
        {
            uint8_t regs[6];

            /* Beim Schreiben Werte prüfen */
            if (cmdline_write)
            {
                int scanargs[6];

                /* Register-Werte korrekt angegeben? */
                if (6 != sscanf(cmdline_regset, "%x,%x,%x,%x,%x,%x",
                    &scanargs[0], &scanargs[1], &scanargs[2], &scanargs[3], &scanargs[4], &scanargs[5]))
                {
                    printf ("--regset braucht 6 Register (hex, getrennt durch Komma)\n");
                }
                else
                {
                    regs[0] = scanargs[0];
                    regs[1] = scanargs[1];
                    regs[2] = scanargs[2];
                    regs[3] = scanargs[3];
                    regs[4] = scanargs[4];
                    regs[5] = scanargs[5];

                    softrock_write_virtual_registers (fifisdr, regs);
                }
            }
            else
            {
                bool result = false;;

                if (cmdline_regs)
                {
                    result = softrock_read_registers (fifisdr, regs);
                }
                if (cmdline_vregs)
                {
                    result = softrock_read_virtual_registers (fifisdr, regs);
                }
                if (result)
                {
                    /* Felder HS_DIV, N1 und RFREQ rausziehen */
                    uint32_t hs_div;
                    uint32_t n1;
                    uint32_t rfreq_int;
                    uint32_t rfreq_frac;
                    double rfreq;

                    /* HS_DIV im Register */
                    hs_div = (regs[0] >> 5) & 7;
                    /* Effektiver Wert */
                    hs_div += 4;

                    /* N1 im Register */
                    n1 = ((regs[0] & 0x1F) << 2) | ((regs[1] >> 6) & 3);
                    /* Effektiver Wert */
                    n1 += 1;

                    rfreq_int = regs[1] & 0x3F;
                    rfreq_int = (rfreq_int << 4) | ((regs[2] >> 4) & 0x0F);
                    rfreq_frac = regs[2] & 0x0F;
                    rfreq_frac = (rfreq_frac << 8) | regs[3];
                    rfreq_frac = (rfreq_frac << 8) | regs[4];
                    rfreq_frac = (rfreq_frac << 8) | regs[5];
                    rfreq = (double)rfreq_int + (double)rfreq_frac / pow(2,28);

                    /* Mit Quarzfrequenz kann noch die echte RX-Frequenz bestimmt werden.
                     * Auch der virtuelle VCO-Faktor wird ggf. benötigt.
                     */
                    double rxfreq = 0.0;
                    double xtal;
                    uint32_t factor;
                    if (softrock_read_xtal (fifisdr, &xtal))
                    {
                        if (cmdline_vregs)
                        {
                            if (softrock_read_virtual_vco_factor (fifisdr, &factor))
                            {
                                rxfreq = ((rfreq * xtal) / (hs_div * n1)) / factor;
                            }
                        }
                        else
                        {
                            rxfreq = ((rfreq * xtal) / (hs_div * n1)) / 4.0;
                        }
                    }

                    printf ("Si570-Register 7-12: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X"
                            ", HS_DIV=%d, N1=%d, RFREQ=%0.9f (%f MHz)%s\n",
                             regs[0], regs[1], regs[2], regs[3], regs[4], regs[5],
                             hs_div, n1, rfreq, rxfreq,
                             cmdline_vregs ? " (virtuell)" : "");
                }
            }
        }

        /* Faktor für virtuelle Register */
        if (cmdline_virtual_vco_factor)
        {
            /* Beim Schreiben Werte prüfen */
            if (cmdline_write)
            {
                if (cmdline_factor < 0)
                {
                    printf ("Kein Faktor angegeben (--factor)\n");
                }
                else
                {
                    softrock_write_virtual_vco_factor (fifisdr, cmdline_factor);
                }
            }
            else
            {
                uint32_t factor;
                if (softrock_read_virtual_vco_factor (fifisdr, &factor))
                {
                    printf ("Faktor für virtuellen VCO = %" PRIu32 "\n", factor);
                }
            }
        }

        /* Startup-Frequenz */
        if (cmdline_startup)
        {
            /* Beim Schreiben Werte prüfen */
            if (cmdline_write)
            {
                if (cmdline_freq < 0.0)
                {
                    printf ("Keine Frequenz angegeben\n");
                }
                else
                {
                    softrock_write_startup (fifisdr, cmdline_freq);
                }
            }
            else
            {
                double f;
                if (softrock_read_startup (fifisdr, &f))
                {
                    printf ("Startup-Frequenz = %f MHz\n", f);
                }
            }
        }

        /* Smooth Tune */
        if (cmdline_smoothtune)
        {
            uint16_t ppm;

            /* Beim Schreiben Werte prüfen */
            if (cmdline_write)
            {
                if (cmdline_ppm < 0)
                {
                    printf ("Kein ppm-Wert angegeben\n");
                }
                else
                {
                    ppm = cmdline_ppm;
                    softrock_write_smoothtune (fifisdr, ppm);
                }
            }
            else
            {
                if (softrock_read_smoothtune (fifisdr, &ppm))
                {
                    printf ("Smooth Tune = %d ppm\n", (int)ppm);
                }
            }
        }

        /* Autotune für Quarzfrequenz */
        if (cmdline_autotune) {
            uint8_t regs[6];
            bool result = false;;

            /* Factory defaults lesen */
            result = softrock_read_factory_default_registers (fifisdr, regs);

            if (result) {
                /* Felder HS_DIV, N1 und RFREQ rausziehen */
                uint32_t hs_div;
                uint32_t n1;
                uint32_t rfreq_int;
                uint32_t rfreq_frac;
                double rfreq;

                /* HS_DIV im Register */
                hs_div = (regs[0] >> 5) & 7;
                /* Effektiver Wert */
                hs_div += 4;

                /* N1 im Register */
                n1 = ((regs[0] & 0x1F) << 2) | ((regs[1] >> 6) & 3);
                /* Effektiver Wert */
                n1 += 1;

                rfreq_int = regs[1] & 0x3F;
                rfreq_int = (rfreq_int << 4) | ((regs[2] >> 4) & 0x0F);
                rfreq_frac = regs[2] & 0x0F;
                rfreq_frac = (rfreq_frac << 8) | regs[3];
                rfreq_frac = (rfreq_frac << 8) | regs[4];
                rfreq_frac = (rfreq_frac << 8) | regs[5];
                rfreq = (double)rfreq_int + (double)rfreq_frac / pow(2,28);

                /* Mit Quarzfrequenz kann noch die echte RX-Frequenz bestimmt werden. */
                double rxfreq = 0.0;
                double xtal = 114.285;  /* Mit nomineller Quarzfrequenz rechnen! */
                rxfreq = ((rfreq * xtal) / (hs_div * n1));

                /* Annahme: Startup-Frequenz ist im Raster 100 kHz.
                 * Abweichung von rxfreq zu diesem Raster bestimmen. Daraus Korrekturfaktor für XTAL ableiten.
                 * (Per Kommandozeile kann geratene Sollfrequenz überschrieben werden).
                 */
                double rasterfreq = (double)((uint32_t)((rxfreq + 0.05) / 0.1)) * 0.1;
                if (cmdline_freq >= 0.0) {
                    rasterfreq = cmdline_freq;
                }
                double newxtal = xtal * (rasterfreq / rxfreq);
                bool xtalInRange = (newxtal >= 113.0) && (newxtal <= 115.0);

                /* Wenn jemand die RX-Frequenz angegeben hat, dann kann evtl. der Vierfache Wert zum Ziel führen. */
                if (!xtalInRange) {
                    if (cmdline_freq >= 0.0) {
                        double newxtal4 = xtal * ((4.0 * rasterfreq) / rxfreq);
                        xtalInRange = (newxtal4 >= 113.0) && (newxtal4 <= 115.0);
                        if (xtalInRange) {
                            rasterfreq = 4.0 * rasterfreq;
                            newxtal = newxtal4;
                        }
                    }
                }

                if (cmdline_write) {
                    if (!xtalInRange) {
                        printf("Unzulässiger XTAL-Wert (%f MHz) NICHT geschrieben. (113 MHz <= XTAL <= 115 MHz)\n",
                               newxtal);
                    }
                    else {
                        softrock_write_xtal (fifisdr, newxtal);
                    }
                }
                else {
                    printf("Factory-Startup: %f MHz (soll: %f MHz), Vorschlag: XTAL=%f\n",
                           rxfreq, rasterfreq, newxtal);
                }
            }
        }

        /* Version-String auslesen */
        if (cmdline_firmware) {
            const uint32_t length = 255;
			uint32_t svn = 0;
            char version[length + 1];
            memset(version, 0, sizeof(version));

            if (softrock_read_version_number (fifisdr, &svn)) {
                printf ("Firmware-Stand (SVN) = %d\n", (int)svn);
            }

            if (softrock_read_version_string (fifisdr, version, length)) {
                printf ("Firmware-Bezeichnung = %s\n", version);
            }
        }
 
        /* Debug-Werte auslesen */
        if (cmdline_debuginfo) {
			union {
				uint32_t d[8];
				uint16_t w[16];
				uint8_t b[32];
			} debug;
            memset(&debug, 0, sizeof(debug));

            if (softrock_read_debuginfo (fifisdr, (uint8_t *)&debug, 32)) {
                printf ("Debug: 0x%08X:0x%08X:0x%08X:0x%08X:0x%08X:0x%08X:0x%08X:0x%08X\n",
                                debug.d[0], debug.d[1], debug.d[2], debug.d[3],
                                debug.d[4], debug.d[5], debug.d[6], debug.d[7]);
            }
        }
 
        /* Volume setzen */
        if (cmdline_volume != -1) {
			if (cmdline_volume != 0) {
				cmdline_volume = 0x0600;
			}

			if (cmdline_write) {
            	softrock_write_volume(fifisdr, cmdline_volume);
			}
			else {
				printf("Volume kann nur geschrieben werden\n!");
			}
        }
 
        /* Demodulator-Mode abfragen/setzen */
		if (cmdline_demod) {
			uint8_t demodMode = 255;

			if (!strcmp(cmdline_demodmode, "lsb") || !strcmp(cmdline_demodmode, "LSB")) {
				demodMode = 0;
			}
			if (!strcmp(cmdline_demodmode, "usb") || !strcmp(cmdline_demodmode, "USB")) {
				demodMode = 1;
			}
			if (!strcmp(cmdline_demodmode, "am") || !strcmp(cmdline_demodmode, "AM")) {
				demodMode = 2;
			}
			if (!strcmp(cmdline_demodmode, "fm") || !strcmp(cmdline_demodmode, "FM")) {
				demodMode = 3;
			}
			if (!strcmp(cmdline_demodmode, "wbfm") || !strcmp(cmdline_demodmode, "WBFM")) {
				demodMode = 4;
			}

			if (cmdline_write) {
				if (demodMode == 255) {
					printf("Kein Modus (\"LSB\", \"USB\", \"AM\", \"FM\", \"WBFM\") angegeben\n");
				}
	        	softrock_write_demodulator_mode(fifisdr, demodMode);
			}
			else {
	        	if (softrock_read_demodulator_mode(fifisdr, &demodMode)) {
					char *modeName = "UNBEKANNT";
					switch (demodMode) {
						case 0: modeName = "LSB"; break;
						case 1: modeName = "USB"; break;
						case 2: modeName = "AM"; break;
						case 3: modeName = "FM"; break;
						case 4: modeName = "WBFM"; break;
					}
					printf("Demodulator: %s\n", modeName);
				}
			}
		}
 
        /* Offset abfragen/setzen */
		if (cmdline_offset) {
			int32_t subtract1121;
			uint32_t multiply1121;

			/* Zunächst aktuellen Stand lesen */
            if (softrock_read_subtract_multiply(fifisdr, &subtract1121, &multiply1121)) {
				if (cmdline_write) {
					if ((cmdline_subtract >= -1.0) && (cmdline_subtract <= +1.0)) {
						subtract1121 = (int32_t)(cmdline_subtract * (4.0 * 2097152.0));
					}
					if (cmdline_multiply >= 0.0) {
						multiply1121 = (uint32_t)(cmdline_multiply * 2097152.0);
					}
		            softrock_write_subtract_multiply(fifisdr, subtract1121, multiply1121);
				}
				else {
					double sub = subtract1121 / (4.0 * 2097152.0);
					double mul = multiply1121 / 2097152.0;

	                printf ("Subtract: %f MHz, Multiply: %f\n", sub, mul);
				}
            }
		}

        /* Demodulator-Bandbreite */
        if (cmdline_bandwidth)
        {
            uint32_t freq;

            /* Beim Schreiben Werte prüfen */
            if (cmdline_write)
            {
                if (cmdline_freq < 0.0)
                {
                    printf ("Keine Frequenz (--freq in Hz) angegeben\n");
                }
                else
                {
                    freq = (uint32_t)cmdline_freq;
                    if (softrock_write_bandwidth (fifisdr, freq)) {
						uint32_t freq2;
		                if (softrock_read_bandwidth (fifisdr, &freq2))
		                {
							if (freq != freq2) {
								printf("Warnung: Bandbreite: %d Hz\n", freq2);
							}
						}
					}
                }
            }
            else
            {
                if (softrock_read_bandwidth (fifisdr, &freq))
                {
                    printf ("Bandbreite: %d Hz\n", freq);
                }
            }
        }
 
 		libusb_close (fifisdr);
    }
    else {
        if (!cmdline_list_devices) {
            printf ("Kein passendes FiFi-SDR gefunden\n" );
        }
    }

    /* Nach dem Schreiben etwas warten, damit das FiFi-SDR Zeit hat das Flash zu beschreiben. */
    if (cmdline_write)
    {
        usleep (200000);    /* 200 ms */
    }

    libusb_exit (usb_context);

    return (0);
}
