#include <iostream>
#include <boost/asio.hpp>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "serial_utils.hpp"

using namespace std;
using namespace boost::asio;

// ฟังก์ชันสำหรับการดึงรายการพอร์ตจากคำสั่ง `ls /dev/tty.*`
vector<string> list_ports() {
    vector<string> ports;
    char buffer[128];

    // เรียกคำสั่ง ls /dev/tty.* และดึงผลลัพธ์
    FILE* fp = popen("ls /dev/tty.*", "r");
    if (fp == nullptr) {
        cerr << "Failed to run command" << endl;
        return ports;
    }

    // อ่านผลลัพธ์ที่ได้จากคำสั่ง ls
    while (fgets(buffer, sizeof(buffer), fp) != nullptr) {
        string port(buffer);
        port.erase(port.find_last_not_of("\n") + 1);  // ลบ newline ออก
        ports.push_back(port);
    }

    fclose(fp);
    return ports;
}

int main() {
    try {
        io_service io;
        vector<string> available_ports = list_ports();

        if (available_ports.empty()) {
            cerr << "No serial ports found!" << endl;
            return -1;
        }

        // แสดงรายการพอร์ตที่มีอยู่
        cout << "Available serial ports:" << endl;
        for (size_t i = 0; i < available_ports.size(); ++i) {
            cout << i + 1 << ". " << available_ports[i] << endl;
        }

        // ให้ผู้ใช้เลือกพอร์ต
        int choice;
        cout << "Enter the number of the port to connect: ";
        cin >> choice;

        if (choice < 1 || choice > available_ports.size()) {
            cerr << "Invalid selection!" << endl;
            return -1;
        }

        string selected_port = available_ports[choice - 1];
        serial_port serial(io, selected_port);
        serial.set_option(serial_port_base::baud_rate(9600));
        serial.set_option(serial_port_base::character_size(8));
        serial.set_option(serial_port_base::parity(serial_port_base::parity::none));
        serial.set_option(serial_port_base::stop_bits(serial_port_base::stop_bits::one));
        serial.set_option(serial_port_base::flow_control(serial_port_base::flow_control::none));

        if (!serial.is_open()) {
            cerr << "Failed to open serial port!" << endl;
            return -1;
        }

        cout << "Serial port connected." << endl;

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
        while (true) {
            string voltage_response, current_response;

            send_scpi_command(serial, "MEAS:VOLT?", voltage_response);
            try {
                double measured_voltage = stod(voltage_response);
                cout << "Measured Voltage: " << measured_voltage << " V" << endl;
            } catch (const exception& e) {
                cerr << "Failed to parse voltage: " << e.what() << endl;
            }

            send_scpi_command(serial, "MEAS:CURRent?", current_response);
            try {
                double measured_current = stod(current_response);
                cout << "Measured Current: " << measured_current << " A" << endl;
            } catch (const exception& e) {
                cerr << "Failed to parse current: " << e.what() << endl;
            }

            this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::exception& e) {
        cerr << "Error: " << e.what() << endl;
    }

    return 0;
}