#include <iostream>
#include <boost/asio.hpp>
#include "serial_utils.hpp"
#include "modbus_utils.hpp"
#include <thread>
#include <chrono>

using namespace std;
using namespace boost::asio;

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
            return -1; 
        }

        string selected_port_ps = available_ports[selected_index_ps];

        serial_port serial = init_serial_port(io, selected_port_ps);

        cout << "----------------------------" << endl;
        cout << "selecting port modbus sensor :" << endl;
        cout << "----------------------------" << endl;
        int selected_index_mb = select_port(available_ports);
        if (selected_index_mb == -1)
        {
            return -1;
        }
        
        const char* selected_port_mb = available_ports[selected_index_mb].c_str();
        modbus_t *ctx = initialize_modbus(selected_port_mb);
        if (ctx == nullptr)
        {
            return -1;
        }

        string response;
        send_scpi_command(serial, "*IDN?", response);
        cout << "IDN Response: " << response << endl;

        double SetPointFlow;
        cout << "Enter set point Flow (in l/min): ";
        cin >> SetPointFlow;

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
