#include "Xcept.h"

#include <libftdi1/ftdi.h>

class ftdi_device {
    public:
        ftdi_device() {
            con = ftdi_new();
            if (!con)
                throw Xcept("ftdi_new allocation failed");
        }
        ~ftdi_device() {
            ftdi_free(con);
        }

        struct ftdi_context *get_context() {
            return con;
        }

        // methods
        void usb_open(int vendor, int product) {
            if (ftdi_usb_open(con, vendor, product) != 0)
                throw Xcept("ftdi_usb_open: %s", ftdi_get_error_string(con));
        }

        void usb_close() {
            if (ftdi_usb_close(con) != 0)
                throw Xcept("ftdi_usb_close: %s", ftdi_get_error_string(con));
        }

        void usb_reset() {
            if (ftdi_usb_reset(con) != 0)
                throw Xcept("ftdi_usb_reset: %s", ftdi_get_error_string(con));
        }

        void usb_purge_buffers() {
            if (ftdi_usb_purge_buffers(con) != 0)
                throw Xcept("ftdi_usb_purge_buffers: %s", ftdi_get_error_string(con));
        }

        void set_bitmode(unsigned char bitmask, unsigned char mode) {
            if (ftdi_set_bitmode(con, bitmask, mode) != 0)
                throw Xcept("ftdi_set_bitmode: %s", ftdi_get_error_string(con));
        }

        void set_latency_timer(unsigned char latency) {
            if (ftdi_set_latency_timer(con, latency) != 0)
                throw Xcept("ftdi_set_latency_timer: %s", ftdi_get_error_string(con));
        }

        void read_data_set_chunksize(unsigned int chunksize) {
            if (ftdi_read_data_set_chunksize(con, chunksize) != 0)
                throw Xcept("ftdi_read_data_set_chunksize: %s", ftdi_get_error_string(con));
        }

        void write_data_set_chunksize(unsigned int chunksize) {
            if (ftdi_write_data_set_chunksize(con, chunksize) != 0)
                throw Xcept("ftdi_write_data_set_chunksize: %s", ftdi_get_error_string(con));
        }

        void set_flowctrl(int flowctrl) {
            if (ftdi_setflowctrl(con, flowctrl) != 0)
                throw Xcept("ftdi_setflowctrl: %s", ftdi_get_error_string(con));
        }

        int read_data(unsigned char *buf, int size) {
            return ftdi_read_data(con, buf, size);
        }

        int write_data(unsigned char *buf, int size) {
            return ftdi_write_data(con, buf, size);
        }
    private:
        struct ftdi_context *con;
};
