#include <modbus/modbus.h>
#include <iostream>
#include <errno.h>
#include <cstring>  // สำหรับ memcpy ใช้สำหรับรวมข้อมูลจาก 2 register มาเป็นข้อมูลชุดเดียว
#include <thread>   // สำหรับ std::this_thread::sleep_for
#include <chrono>   // สำหรับ std::chrono::seconds

int main() {

    // กำหนดการเชื่อมต่อ Modbus RTU
    const char* device = "/dev/tty.usbserial-130"; // เปลี่ยนเป็นชื่อพอร์ตที่ใช้งานจริง เช่น /dev/tty.usbserial-123456
    int baud_rate = 19200;               // ความเร็วในการเชื่อมต่อ
    char parity = 'N';                  // การตั้งค่าพาริตี้ ('N' = None, 'E' = Even, 'O' = Odd)
    int data_bit = 8;                   // จำนวนบิตข้อมูล
    int stop_bit = 1;                   // จำนวนบิตหยุด

    // สร้าง context สำหรับ Modbus RTU
    modbus_t* ctx = modbus_new_rtu(device, baud_rate, parity, data_bit, stop_bit);
    if (ctx == nullptr) {
        std::cerr << "Failed to create Modbus context." << std::endl;
        return -1;
    }

    // ตั้ง slave ID ของอุปกรณ์ (ในตัวอย่างคือ 1)
    int slave_id = 2; // Slave ID ของอุปกรณ์ที่ต้องการติดต่อ
    if (modbus_set_slave(ctx, slave_id) == -1) {
        std::cerr << "Failed to set slave ID: " << modbus_strerror(errno) << std::endl;
        modbus_free(ctx);
        return -1;
    }

    // ตั้งค่า Timeout (1 วินาที)
    uint32_t timeout_sec = 1;  // วินาที
    uint32_t timeout_usec = 0; // มิลลิวินาที
    if (modbus_set_response_timeout(ctx, timeout_sec, timeout_usec) == -1) {
        std::cerr << "Failed to set response timeout: " << modbus_strerror(errno) << std::endl;
        modbus_free(ctx);
        return -1;
    }

    // เปิดการเชื่อมต่อกับอุปกรณ์
    if (modbus_connect(ctx) == -1) {
        std::cerr << "Connection failed: " << modbus_strerror(errno) << std::endl;
        modbus_free(ctx);
        return -1;
    }
    
    while(true) {
        uint16_t register_value[4]; // อ่าน 4 รีจิสเตอร์ (2 รีจิสเตอร์สำหรับแต่ละค่าที่เป็น float หรือ uint32_t)

        // อ่านค่าจากรีจิสเตอร์ที่อยู่ 6 (flow)
        int rc_flow;
        do {
            rc_flow = modbus_read_registers(ctx, 6, 2, register_value); // อ่าน 2 รีจิสเตอร์ที่อยู่ 6
            if (rc_flow == -1) {
                // std::cerr << "Failed to read flow register: " << modbus_strerror(errno) << ". Retrying..." << std::endl;
                // std::this_thread::sleep_for(std::chrono::seconds(1)); // หน่วงเวลา 1 วินาที
            }
        } while (rc_flow == -1); // ลองใหม่หากเกิด error

        float flow;
        memcpy(&flow, register_value, sizeof(flow)); // แปลงค่าเป็น float
        std::cout << "Flow (Address 6) = " << flow / 10.0f << std::endl; // แบ่งด้วย 10 เนื่องจาก resolution เป็น 0.1

        // อ่านค่าจากรีจิสเตอร์ที่อยู่ 8 (Consumption)
        int rc_consumption;
        do {
            rc_consumption = modbus_read_registers(ctx, 8, 2, register_value); // อ่าน 2 รีจิสเตอร์ที่อยู่ 8
            if (rc_consumption == -1) {
                // std::cerr << "Failed to read consumption register: " << modbus_strerror(errno) << ". Retrying..." << std::endl;
                // std::this_thread::sleep_for(std::chrono::seconds(1)); // หน่วงเวลา 1 วินาที
            }
        } while (rc_consumption == -1); // ลองใหม่หากเกิด error

        uint32_t consumption;
        memcpy(&consumption, register_value, sizeof(consumption)); // แปลงค่าเป็น uint32_t
        std::cout << "Consumption (Address 8) = " << consumption << std::endl; // แสดงค่า

        // อ่านค่าจากรีจิสเตอร์ที่อยู่ 2 (Pressure)
        int rc_pressure;
        do {
            rc_pressure = modbus_read_registers(ctx, 2, 2, register_value); // อ่าน 2 รีจิสเตอร์ที่อยู่ 2
            if (rc_pressure == -1) {
                // std::cerr << "Failed to read pressure register: " << modbus_strerror(errno) << ". Retrying..." << std::endl;
                // std::this_thread::sleep_for(std::chrono::seconds(1)); // หน่วงเวลา 1 วินาที
            }
        } while (rc_pressure == -1); // ลองใหม่หากเกิด error

        float pressure;
        memcpy(&pressure, register_value, sizeof(pressure)); // แปลงค่าเป็น float
        std::cout << "Pressure (Address 2) = " << pressure / 100.0f << std::endl; // แบ่งด้วย 100 เนื่องจาก resolution เป็น 0.01

        std::cout << "--------------------------------" << std::endl;

        // หน่วงเวลา 1 วินาที
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // ปิดการเชื่อมต่อ
    modbus_close(ctx);
    modbus_free(ctx);

    return 0;
}