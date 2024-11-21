#include "modbus_utils.hpp"
#include <iostream>
#include <errno.h>
#include <cstring>

modbus_t* initialize_modbus(const char* device)
{
    // ค่าคงที่ที่ใช้ในการกำหนดการเชื่อมต่อ Modbus
    int baud_rate = 19200;
    char parity = 'N';
    int data_bit = 8;
    int stop_bit = 1;
    int slave_id = 2;

    // สร้าง context สำหรับ Modbus RTU
    modbus_t* ctx = modbus_new_rtu(device, baud_rate, parity, data_bit, stop_bit);
    if (ctx == nullptr)
    {
        std::cerr << "Failed to create Modbus context." << std::endl;
        return nullptr;
    }

    // ตั้ง slave ID ของอุปกรณ์
    if (modbus_set_slave(ctx, slave_id) == -1)
    {
        std::cerr << "Failed to set slave ID: " << modbus_strerror(errno) << std::endl;
        modbus_free(ctx);
        return nullptr;
    }

    // ตั้งค่า Timeout
    uint32_t timeout_sec = 1;
    uint32_t timeout_usec = 0;
    if (modbus_set_response_timeout(ctx, timeout_sec, timeout_usec) == -1)
    {
        std::cerr << "Failed to set response timeout: " << modbus_strerror(errno) << std::endl;
        modbus_free(ctx);
        return nullptr;
    }

    // เปิดการเชื่อมต่อกับอุปกรณ์
    if (modbus_connect(ctx) == -1)
    {
        std::cerr << "Connection failed: " << modbus_strerror(errno) << std::endl;
        modbus_free(ctx);
        return nullptr;
    }

    return ctx;
}
