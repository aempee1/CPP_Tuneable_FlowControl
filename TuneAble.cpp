#include <iostream>
#include <boost/asio.hpp>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "serial_utils.hpp"

//---------------------------
#include <modbus/modbus.h>
#include <errno.h>
#include <cstring> // สำหรับ memcpy ใช้สำหรับรวมข้อมูลจาก 2 register มาเป็นข้อมูลชุดเดียว
#include <thread>  // สำหรับ std::this_thread::sleep_for
#include <chrono>  // สำหรับ std::chrono::seconds

using namespace std;
using namespace boost::asio;

// ฟังก์ชันสำหรับการดึงรายการพอร์ตจากคำสั่ง `ls /dev/tty.*`
vector<string> list_ports()
{
    vector<string> ports;
    char buffer[128];

    // เรียกคำสั่ง ls /dev/tty.* และดึงผลลัพธ์
    FILE *fp = popen("ls /dev/tty.*", "r");
    if (fp == nullptr)
    {
        cerr << "Failed to run command" << endl;
        return ports;
    }

    // อ่านผลลัพธ์ที่ได้จากคำสั่ง ls
    while (fgets(buffer, sizeof(buffer), fp) != nullptr)
    {
        string port(buffer);
        port.erase(port.find_last_not_of("\n") + 1); // ลบ newline ออก
        ports.push_back(port);
    }

    fclose(fp);
    return ports;
}

// ฟังก์ชันสำหรับแสดงรายการพอร์ตและให้ผู้ใช้เลือก
int select_port(const vector<string> &available_ports)
{
    if (available_ports.empty())
    {
        cerr << "No serial ports found!" << endl;
        return -1;
    }

    // แสดงรายการพอร์ตที่มีอยู่
    cout << "Available serial ports:" << endl;
    for (size_t i = 0; i < available_ports.size(); ++i)
    {
        cout << i + 1 << ". " << available_ports[i] << endl;
    }

    // ให้ผู้ใช้เลือกพอร์ต
    int choice;
    cout << "Enter the number of the port to connect : ";
    cin >> choice;

    if (choice < 1 || choice > available_ports.size())
    {
        cerr << "Invalid selection!" << endl;
        return -1;
    }

    return choice - 1; // ส่งคืน index ของพอร์ตที่เลือก
}

// ฟังก์ชันสำหรับการตั้งค่า serial_port
serial_port init_serial_port(io_service &io, const string &port_name)
{
    serial_port serial(io, port_name);
    serial.set_option(serial_port_base::baud_rate(9600));
    serial.set_option(serial_port_base::character_size(8));
    serial.set_option(serial_port_base::parity(serial_port_base::parity::none));
    serial.set_option(serial_port_base::stop_bits(serial_port_base::stop_bits::one));
    serial.set_option(serial_port_base::flow_control(serial_port_base::flow_control::none));

    if (!serial.is_open())
    {
        cerr << "Failed to open serial port!" << endl;
        throw runtime_error("Failed to open serial port");
    }

    cout << "Serial port connected." << endl;
    return serial;
}

//----------------------------------------------------------------

// ฟังก์ชันสำหรับการตั้งค่าการเชื่อมต่อ Modbus (รับแค่ค่า device)
modbus_t *initialize_modbus(const char *device)
{
    // ค่าคงที่ที่ใช้ในการกำหนดการเชื่อมต่อ Modbus
    int baud_rate = 19200; // ความเร็วในการเชื่อมต่อ
    char parity = 'N';     // การตั้งค่าพาริตี้ ('N' = None, 'E' = Even, 'O' = Odd)
    int data_bit = 8;      // จำนวนบิตข้อมูล
    int stop_bit = 1;      // จำนวนบิตหยุด
    int slave_id = 2;      // Slave ID ของอุปกรณ์ที่ต้องการติดต่อ

    // สร้าง context สำหรับ Modbus RTU
    modbus_t *ctx = modbus_new_rtu(device, baud_rate, parity, data_bit, stop_bit);
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

    // ตั้งค่า Timeout (1 วินาที)
    uint32_t timeout_sec = 1;  // วินาที
    uint32_t timeout_usec = 0; // มิลลิวินาที
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

int main()
{
    try
    {
        io_service io;
        vector<string> available_ports = list_ports();
        cout << "----------------------------" << endl;
        cout << "Selecting port power supply :" << endl;
        cout << "----------------------------" << endl;
        int selected_index_ps = select_port(available_ports);
        if (selected_index_ps == -1)
        {
            return -1; // จบโปรแกรมถ้าไม่มีพอร์ตหรือเลือกผิด
        }

        string selected_port_ps = available_ports[selected_index_ps];

        // เรียกใช้ฟังก์ชัน init_serial_port เพื่อเปิดและตั้งค่า serial port
        serial_port serial = init_serial_port(io, selected_port_ps);
        //----------------------------------------------------------------
        cout << "----------------------------" << endl;
        cout << "selecting port modbus sensor :" << endl;
        cout << "----------------------------" << endl;
        int selected_index_mb = select_port(available_ports);
        if (selected_index_mb == -1)
        {
            return -1; // จบโปรแกรมถ้าไม่มีพอร์ตหรือเลือกผิด
        }
         // เลือกพอร์ตที่ผู้ใช้เลือก
        const char* selected_port_mb = available_ports[selected_index_mb].c_str(); // ตัวอย่างการเลือกพอร์ต
        // เรียกฟังก์ชัน initialize_modbus โดยส่งค่า device เป็นพารามิเตอร์
        modbus_t *ctx = initialize_modbus(selected_port_mb);
        if (ctx == nullptr)
        {
            return -1;
        }

        //----------------------------------------------------------------
        string response;
        send_scpi_command(serial, "*IDN?", response);
        cout << "IDN Response: " << response << endl;

        // ให้ผู้ใช้กำหนดค่า
        double voltage, current;
        cout << "Enter voltage to set (in V): ";
        cin >> voltage;
        set_voltage(serial, voltage);

        cout << "Enter current to set (in A): ";
        cin >> current;
        set_current(serial, current);

        // อ่านค่าแบบต่อเนื่อง
        while (true)
        {
            string voltage_response, current_response;

            send_scpi_command(serial, "MEAS:VOLT?", voltage_response);
            try
            {
                double measured_voltage = stod(voltage_response);
                cout << "Measured Voltage: " << measured_voltage << " V" << endl;
            }
            catch (const exception &e)
            {
                cerr << "Failed to parse voltage: " << e.what() << endl;
            }

            send_scpi_command(serial, "MEAS:CURRent?", current_response);
            try
            {
                double measured_current = stod(current_response);
                cout << "Measured Current: " << measured_current << " A" << endl;
            }
            catch (const exception &e)
            {
                cerr << "Failed to parse current: " << e.what() << endl;
            }
            cout << "----------------------------------------------------------------" << endl;
            this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    catch (const std::exception &e)
    {
        cerr << "Error: " << e.what() << endl;
    }

    return 0;
}