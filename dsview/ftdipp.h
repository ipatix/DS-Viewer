#pragma once

#include "Xcept.h"

#include <string>
#include <vector>
#include <cassert>
#include <cstdint>

#ifdef __unix__
#include <libftdi1/ftdi.h>

class ftdi_device {
    private:
        struct ftdi_context *con;
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
            if (ftdi_tcioflush(con) != 0)
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

        void data_set_chunksize(unsigned int read_size, unsigned int write_size) {
            if (ftdi_read_data_set_chunksize(con, read_size) != 0)
                throw Xcept("ftdi_read_data_set_chunksize: %s", ftdi_get_error_string(con));
            if (ftdi_write_data_set_chunksize(con, write_size) != 0)
                throw Xcept("ftdi_write_data_set_chunksize: %s", ftdi_get_error_string(con));
        }

        void set_flowctrl(int flowctrl, unsigned char xon, unsigned char xoff) {
            (void)xon;
            (void)xoff;
            if (ftdi_setflowctrl(con, flowctrl) != 0)
                throw Xcept("ftdi_setflowctrl: %s", ftdi_get_error_string(con));
        }

        void set_timeouts(int read_timeout, int write_timeout) {
            con->usb_read_timeout = read_timeout;
            con->usb_write_timeout = write_timeout;
        }

        int read_data(unsigned char *buf, int size) {
            return ftdi_read_data(con, buf, size);
        }

        int write_data(unsigned char *buf, int size) {
            return ftdi_write_data(con, buf, size);
        }
};

#elif _WIN32
#include "ftd2xx.h"
#include "WinTypes.h"

class ftdi_device
{
    private:
        static std::string getErrText(FT_STATUS err) {
            switch (err) {
                case FT_OK:
                    return "Success";
                case FT_INVALID_HANDLE:
                    return "Invalid Handle";
                case FT_DEVICE_NOT_FOUND:
                    return "Device not found";
                case FT_DEVICE_NOT_OPENED:
                    return "Device not opened";
                case FT_IO_ERROR:
                    return "IO Error";
                case FT_INSUFFICIENT_RESOURCES:
                    return "Insufficient Resources";
                case FT_INVALID_PARAMETER:
                    return "Invalid Parameter";
                case FT_INVALID_BAUD_RATE:
                    return "Invalid Baudrate";

                case FT_DEVICE_NOT_OPENED_FOR_ERASE:
                    return "Device not opened for erase";
                case FT_DEVICE_NOT_OPENED_FOR_WRITE:
                    return "Device not opened for write";
                case FT_FAILED_TO_WRITE_DEVICE:
                    return "Failed to write Device";
                case FT_EEPROM_READ_FAILED:
                    return "EEPROM read failed";
                case FT_EEPROM_WRITE_FAILED:
                    return "EEPROM write failed";
                case FT_EEPROM_ERASE_FAILED:
                    return "EEPROM erase failed";
                case FT_EEPROM_NOT_PRESENT:
                    return "EEPROM not present";
                case FT_EEPROM_NOT_PROGRAMMED:
                    return "EEPROM not programmed";
                case FT_INVALID_ARGS:
                    return "Invalid Arguments";
                case FT_NOT_SUPPORTED:
                    return "Not supported";
                case FT_OTHER_ERROR:
                    return "Other Error";
                case FT_DEVICE_LIST_NOT_READY:
                    return "Device Kist not ready";
                default:
                    return "FATAL: Invalid Error Code";
            }
        }    

        static DWORD numDevices;
        static std::vector<FT_DEVICE_LIST_INFO_NODE> devices;

        // replaces FT_CreateDeviceInfoList
        static void updateDeviceIndex() {
            FT_STATUS err;
            if ((err = FT_CreateDeviceInfoList(&numDevices)) != FT_OK)
                throw Xcept("FT_CreateDeviceInfoList: %s", getErrText(err));
        }

        FT_HANDLE handle;
        bool closed;
    
    public:
        // FT_Open device 0
        ftdi_device() {
            closed = true;
        }

        ~ftdi_device() {
            if (!closed)
                FT_Close(handle);
        }

        void usb_open() {
            FT_STATUS err;
            if ((err = FT_Open(0, &handle)) != FT_OK)
                throw Xcept("FT_Open: %s", getErrText(err));
            closed = false;
        }

        void usb_open(int deviceIndex) {
            FT_STATUS err;
            if ((err = FT_Open(deviceIndex, &handle)) != FT_OK)
                throw Xcept("FT_Open: %s", getErrText(err));
            closed = false;
        }

        void usb_open(const std::string& name, DWORD flags) {
            FT_STATUS err;
            if ((err = FT_OpenEx((void *)name.c_str(), flags, &handle)) != FT_OK)
                throw Xcept("FT_OpenEx: %s", getErrText(err));
            closed = false;
        }

        void usb_close() {
            FT_STATUS err;
            if ((err = FT_Close(handle)) != FT_OK)
                throw Xcept("FT_Close: %s", getErrText(err));
        }

        // FT_GetDeviceInfoList
        // TODO

        // FT_GetDeviceInfoDetail
        // TODO
        
        // FT_Read
        size_t read_data(unsigned char *data, size_t bytesToRead) {
            DWORD bytes_returned;
            FT_STATUS err;
            if ((err = FT_Read(handle, data, (DWORD)bytesToRead, &bytes_returned)) != FT_OK)
                throw Xcept("FT_Read: %s", getErrText(err));
            return bytes_returned;
        }

        // FT_Write
        size_t write_data(unsigned char *data, size_t bytesToWrite) {
            DWORD bytes_written;
            FT_STATUS err;
            if ((err = FT_Write(handle, data, (DWORD)bytesToWrite, &bytes_written)) != FT_OK)
                throw Xcept("FT_Write: %s", getErrText(err));
            return bytes_written;
        }

        // FT_SetBaudRate
        void SetBaudRate(DWORD baudRate) {
            FT_STATUS err;
            if ((err = FT_SetBaudRate(handle, baudRate)) != FT_OK)
                throw Xcept("FT_SetBaudRate: %s", getErrText(err));
        }

        // FT_SetDivisor
        void SetDivisor(unsigned short divisor) {
            FT_STATUS err;
            if ((err = FT_SetDivisor(handle, divisor)) != FT_OK)
                throw Xcept("FT_SetDivisor: %s", getErrText(err));
        }

        // FT_SetDataCharacteristics
        void SetDataCharacteristics(uint8_t wordLength, uint8_t stopBits, uint8_t parity) {
            FT_STATUS err;
            if ((err = FT_SetDataCharacteristics(handle, wordLength, stopBits, parity)) != FT_OK)
                throw Xcept("FT_SetDataCharacteristics: %s", getErrText(err));
        }

        // FT_SetTimeouts
        void set_timeouts(DWORD readTimeout, DWORD writeTimeout) {
            FT_STATUS err;
            if ((err = FT_SetTimeouts(handle, readTimeout, writeTimeout)) != FT_OK)
                throw Xcept("FT_SetTimeouts: %s", getErrText(err));
        }

        // FT_SetFlowControl
        void set_flowctrl(uint16_t flowControl, uint8_t xon, uint8_t xoff) {
            FT_STATUS err;
            if ((err = FT_SetFlowControl(handle, flowControl, xon, xoff)) != FT_OK)
                throw Xcept("FT_SetFlowControl: %s", getErrText(err));
        }

        // FT_SetDtr
        void SetDtr() {
            FT_STATUS err;
            if ((err = FT_SetDtr(handle)) != FT_OK)
                throw Xcept("FT_SetDtr: %s", getErrText(err));
        }

        // FT_ClrDtr
        void ClrDtr() {
            FT_STATUS err;
            if ((err = FT_ClrDtr(handle)) != FT_OK)
                throw Xcept("FT_ClrDtr: %s", getErrText(err));
        }

        // FT_SetRts
        void SetRts() {
            FT_STATUS err;
            if ((err = FT_SetRts(handle)) != FT_OK)
                throw Xcept("FT_SetRts: %s", getErrText(err));
        }

        // FT_ClrRts
        void ClrRts() {
            FT_STATUS err;
            if ((err = FT_ClrRts(handle)) != FT_OK)
                throw Xcept("FT_ClrRts: %s", getErrText(err));
        }

        // FT_GetModemStatus
        DWORD GetModemStatus() {
            DWORD ret;
            FT_STATUS err;
            if ((err = FT_GetModemStatus(handle, &ret)) != FT_OK)
                throw Xcept("FT_GetModemStatus: %s", getErrText(err));
            return ret;
        }

        // FT_GetQueueStatus
        DWORD GetQueueStatus() {
            DWORD ret;
            FT_STATUS err;
            if ((err = FT_GetQueueStatus(handle, &ret)) != FT_OK)
                throw Xcept("FT_GetQueueStatus: %s", getErrText(err));
            return err;
        }

        // FT_GetDeviceInfo
        void GetDeviceInfo(FT_DEVICE& type, DWORD& deviceID, std::string& serialNumber, std::string& description) {
            FT_STATUS err;
            char serial[16+1]; // sorry for those random numbers, no official documentation
            char desc[64+1];
            if ((err = FT_GetDeviceInfo(handle, &type, &deviceID, serial, desc, NULL)) != FT_OK)
                throw Xcept("FT_GetDeviceInfo: %s", getErrText(err));
            serialNumber = serial;
            description = desc;
        }

        // FT_GetDriverVersion
        DWORD GetDriverVersion() {
            DWORD ret;
            FT_STATUS err;
            if ((err = FT_GetDriverVersion(handle, &ret)) != FT_OK)
                throw Xcept("FT_GetDriverVersion: %s", getErrText(err));
            return ret;
        }

        // FT_GetLibraryVersion
        static DWORD GetLibraryVersion() {
            DWORD ret;
            FT_STATUS err;
            if ((err = FT_GetLibraryVersion(&ret)) != FT_OK)
                throw Xcept("FT_GetLibraryVersion: %s", getErrText(err));
            return ret;
        }

        // FT_GetStatus
        void GetStatus(DWORD& rx, DWORD& tx, DWORD& eventStatus) {
            FT_STATUS err;
            if ((err = FT_GetStatus(handle, &rx, &tx, &eventStatus)) != FT_OK)
                throw Xcept("FT_GetStatus: %s", getErrText(err));
        }

        // FT_SetEventNotification
        // TODO
        
        // FT_SetChars
        void SetChars(uint8_t eventCh, uint8_t eventChEn, uint8_t errorCh, uint8_t errorChEn) {
            FT_STATUS err;
            if ((err = FT_SetChars(handle, eventCh, eventChEn, errorCh, errorChEn)) != FT_OK)
                throw Xcept("FT_SetChars: %s", getErrText(err));
        }

        // FT_SetBreakOn
        void SetBreakOn() {
            FT_STATUS err;
            if ((err = FT_SetBreakOn(handle)) != FT_OK)
                throw Xcept("FT_SetBreakOn: %s", getErrText(err));
        }

        // FT_SetBreakOff
        void SetBreakOff() {
            FT_STATUS err;
            if ((err = FT_SetBreakOff(handle)) != FT_OK)
                throw Xcept("FT_SetBreakOff: %s", getErrText(err));
        }

        // FT_Purge
        void usb_purge_buffers(DWORD mask) {
            FT_STATUS err;
            if ((err = FT_Purge(handle, mask)) != FT_OK)
                throw Xcept("FT_Purge: %s", getErrText(err));
        }

        // FT_ResetDevice
        void usb_reset() {
            FT_STATUS err;
            if ((err = FT_ResetDevice(handle)) != FT_OK)
                throw Xcept("FT_ResetDevice: %s", getErrText(err));
        }

        // FT_StopInTask
        void StopInTask() {
            FT_STATUS err;
            if ((err = FT_StopInTask(handle)) != FT_OK)
                throw Xcept("FT_StopInTask: %s", getErrText(err));
        }

        // FT_RestartInTask
        void RestartInTask() {
            FT_STATUS err;
            if ((err = FT_RestartInTask(handle)) != FT_OK)
                throw Xcept("FT_RestartInTask: %s", getErrText(err));
        }

        // FT_SetDeadmanTimeout
        void SetDeadmanTimeout(DWORD deadmanTimeout) {
            FT_STATUS err;
            if ((err = FT_SetDeadmanTimeout(handle, deadmanTimeout)) != FT_OK)
                throw Xcept("FT_SetDeadmanTimeout: %s", getErrText(err));
        }

        // FT_SetLatencyTimer
        void set_latency_timer(uint8_t timer) {
            FT_STATUS err;
            if ((err = FT_SetLatencyTimer(handle, timer)) != FT_OK)
                throw Xcept("FT_SetLatencyTimer: %s", getErrText(err));
        }

        // FT_GetLatencyTimer
        uint8_t get_latency_timer() {
            uint8_t ret;
            FT_STATUS err;
            if ((err = FT_GetLatencyTimer(handle, &ret)) != FT_OK)
                throw Xcept("FT_GetLatencyTimer: %s", getErrText(err));
            return ret;
        }

        // FT_SetBitMode
        void set_bitmode(uint8_t mask, uint8_t mode) {
            FT_STATUS err;
            if ((err = FT_SetBitMode(handle, mask, mode)) != FT_OK)
                throw Xcept("FT_SetBitMode: %s", getErrText(err));
        }

        // FT_GetBitMode
        uint8_t get_bitmode() {
            uint8_t ret;
            FT_STATUS err;
            if ((err = FT_GetBitMode(handle, &ret)) != FT_OK)
                throw Xcept("FT_GetBitMode: %s", getErrText(err));
            return ret;
        }

        void data_set_chunksize(DWORD inTransferSize, DWORD outTransferSize) {
            FT_STATUS err;
            if ((err = FT_SetUSBParameters(handle, inTransferSize, outTransferSize)) != FT_OK)
                throw Xcept("FT_SetUSBParameters: %s", getErrText(err));
        }
};

#endif
