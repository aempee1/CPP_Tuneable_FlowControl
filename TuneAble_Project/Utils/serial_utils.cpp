#include "serial_utils.hpp"
#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <chrono>
#include <cstdio>


using namespace std;
using namespace boost::asio;


std::vector<std::string> list_ports()
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

int select_port(const vector<string>& available_ports)
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
    cout << "Enter the number of the port to connect: ";
    cin >> choice;

    if (choice < 1 || choice > available_ports.size())
    {
        cerr << "Invalid selection!" << endl;
        return -1;
    }

    return choice - 1; // ส่งคืน index ของพอร์ตที่เลือก
}

boost::asio::serial_port init_serial_port(boost::asio::io_service& io, const std::string& port_name)
{
    boost::asio::serial_port serial(io, port_name);
    serial.set_option(boost::asio::serial_port_base::baud_rate(9600));
    serial.set_option(boost::asio::serial_port_base::character_size(8));
    serial.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
    serial.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
    serial.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));

    if (!serial.is_open())
    {
        cerr << "Failed to open serial port!" << endl;
        throw runtime_error("Failed to open serial port");
    }

    cout << "Serial port connected." << endl;
    return serial;
}

void send_scpi_command(serial_port& serial, const string& command, string& response, bool expect_response ) {
    if (!serial.is_open()) {
        cerr << "Serial port is not open!" << endl;
        return;
    }

    // ส่งคำสั่งไปยัง Serial Port
    write(serial, buffer(command + "\n"));

    // ถ้าคำสั่งไม่ต้องการ response ให้หยุดที่นี่
    if (!expect_response) {
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
        cout << "Command sent: " << command << endl;
        return;
    }

    // ถ้าคำสั่งต้องการ response
    boost::asio::streambuf buffer;
    read_until(serial, buffer, '\n');

    // แปลงข้อมูลที่อ่านได้จาก buffer เป็น string
    istream input_stream(&buffer);
    getline(input_stream, response);

    // เช็คว่ามีข้อมูลหรือไม่
    if (!response.empty()) {
        //cout << "Received response: " << response << endl;
    } else {
        cerr << "No data received or read error" << endl;
    }
}

void set_voltage(serial_port& serial, double voltage) {
    string command = "VOLT " + to_string(voltage);
    string response;
    send_scpi_command(serial, command, response , false);
    cout << "Set Voltage to: " << voltage << " V" << endl;
}

void set_current(serial_port& serial, double current) {
    string command = "CURR " + to_string(current);
    string response;
    send_scpi_command(serial, command, response , false);
    cout << "Set Current to: " << current << " A" << endl;
}