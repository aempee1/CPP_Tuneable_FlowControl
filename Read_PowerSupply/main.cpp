#include <iostream>
#include <boost/asio.hpp>
#include "serial_utils.hpp"

using namespace std;
using namespace boost::asio;

int main() {
    try {
        io_service io;
        serial_port serial(io, "/dev/tty.usbserial-B000OXDY");
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
        send_scpi_command(serial, "*IDN?", response );
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

            send_scpi_command(serial, "MEAS:VOLT?", voltage_response );
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