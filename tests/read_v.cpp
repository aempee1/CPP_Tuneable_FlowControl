#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <chrono>

using namespace std;
using namespace boost::asio;

void send_scpi_command(serial_port& serial, const string& command, string& response) {
    
    // ตรวจสอบว่า Serial Port เปิดใช้งานหรือไม่
    if (!serial.is_open()) {
        cerr << "Serial port is not open!" << endl;
        return;
    }

    // ส่งคำสั่งไปยัง Serial Port
    write(serial, buffer(command + "\n"));
    // cout << "Sent command: " << command << endl;  // แสดงคำสั่งที่ส่งออกไป

    // หน่วงเวลาสักครู่เพื่อให้การตอบกลับจากอุปกรณ์มีเวลา
    std::this_thread::sleep_for(std::chrono::milliseconds(500));  // เพิ่มเวลาหน่วง

    // อ่านผลลัพธ์จาก Serial Port ด้วย read_until
    boost::asio::streambuf buffer;
    read_until(serial, buffer, '\n');  // อ่านจนกว่าจะเจอ \n (หรือจนกว่าจะหมดข้อมูล)

    // แปลงข้อมูลที่อ่านได้จาก buffer เป็น string
    istream input_stream(&buffer);
    getline(input_stream, response);

    // เช็คว่ามีข้อมูลหรือไม่
    if (!response.empty()) {
        // cout << "Received response: " << response << endl;  // แสดงข้อมูลที่ได้รับ
    } else {
        cerr << "No data received or read error" << endl;
    }
}

int main() {
    try {
        io_service io;
        // เปิด Serial Port
        serial_port serial(io, "/dev/tty.usbserial-B000OXDY");
        serial.set_option(serial_port_base::baud_rate(9600));  // กำหนด baud rate
        serial.set_option(serial_port_base::character_size(8));  // กำหนด character size
        serial.set_option(serial_port_base::parity(serial_port_base::parity::none));
        serial.set_option(serial_port_base::stop_bits(serial_port_base::stop_bits::one));
        serial.set_option(serial_port_base::flow_control(serial_port_base::flow_control::none));

        // ตรวจสอบว่า Serial Port เปิดใช้งานสำเร็จหรือไม่
        if (!serial.is_open()) {
            cerr << "Failed to open serial port!" << endl;
            return -1;
        }

        cout << "Serial port connected." << endl;

        // ส่งคำสั่ง SCPI "*IDN?" เพื่อขอข้อมูลอุปกรณ์
        string response;
        send_scpi_command(serial, "*IDN?", response);
        cout << "IDN Response: " << response << endl;

        // อ่านข้อมูลแบบต่อเนื่อง
        while (true) {
            string voltage_response, current_response;

            // อ่านค่าแรงดัน
            send_scpi_command(serial, "MEAS:VOLT?", voltage_response);
            try {
                double voltage = stod(voltage_response);  // แปลงค่าเป็น double
                cout << "Voltage: " << voltage << " V" << endl;
            } catch (const exception& e) {
                cerr << "Failed to parse voltage: " << e.what() << endl;
            }

            // อ่านค่ากระแส
            send_scpi_command(serial, "MEAS:CURRent?", current_response);
            try {
                double current = stod(current_response);  // แปลงค่าเป็น double
                cout << "Current: " << current << " A" << endl;
            } catch (const exception& e) {
                cerr << "Failed to parse current: " << e.what() << endl;
            }

            // หน่วงเวลา 1 วินาที
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::exception& e) {
        cerr << "Error: " << e.what() << endl;
    }

    return 0;
}