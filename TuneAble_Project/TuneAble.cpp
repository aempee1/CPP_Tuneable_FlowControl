#include <iostream>
#include <boost/asio.hpp>
#include "serial_utils.hpp"
#include "modbus_utils.hpp"
#include <fstream>  // สำหรับการเขียนไฟล์
#include <thread>
#include <chrono>
#include <csignal>  // สำหรับการจัดการสัญญาณ
#include <atomic>   // สำหรับตัวแปร thread-safe

using namespace std ;
using namespace boost::asio ;

const double Kp = 0.04 ; // ค่าคงที่ของ PID controller
const double Ki = 0.56 ;
const double Kd = 0.00 ;

double integal = 0.0 ;
double previousError = 0.0 ;
double setPointFlow ;
double current_tunening = 10 ;


// ตัวแปร global เพื่อควบคุมการทำงานของลูป
atomic<bool> running(true); 

// ตัวจัดการสัญญาณ (Signal Handler)
void signal_handler(int signal) {
    if (signal == SIGINT) {
        cout << "\nInterrupt signal (Ctrl+C) received. Exiting safely..." << endl;
        running = false; // สั่งให้ลูปหยุดทำงาน
    }
}

double calculatePID(double setpointValue ,double currentValue)
{
    double errorValue = setpointValue - currentValue;
    integal += errorValue;
    double derivativeValue = errorValue - previousError;
    previousError = errorValue ;
    double PID_output = (Kp* errorValue) + (Kd*integal) + (Kd* derivativeValue);
    return PID_output ;
}

int main()
{
    try
    {
        // จับสัญญาณ Ctrl+C
        signal(SIGINT, signal_handler);

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
        const char *selected_port_mb = available_ports[selected_index_mb].c_str();
        modbus_t *ctx = initialize_modbus(selected_port_mb);
        if (ctx == nullptr)
        {
            return -1;
        }

        // สร้างและเปิดไฟล์ CSV
        ofstream data_file("data_tuning.csv", ios::app);  
        if (!data_file.is_open()) {
            cerr << "Error opening file!" << endl;
            return -1;
        }

        // เขียนหัวข้อถ้าไฟล์ใหม่
        data_file << "Flow,Time\n"; 

        string response;
        send_scpi_command(serial, "*IDN?", response);
        cout << "IDN Response: " << response << endl;

        cout << "Enter set point Flow (in l/min): ";
        cin >> setPointFlow;

        set_voltage(serial, 10.00);
        cout << "set ground state:" << endl;

        string response_open;
        send_scpi_command(serial, "OUTPut ON" ,response , false);

        // บันทึกเวลาเริ่มต้น
        auto start_time = std::chrono::steady_clock::now();

        // ใช้ running เพื่อควบคุมลูป
        while (running)
        {
            uint16_t register_value[4]; 
            int rc_flow;
            do
            {
                rc_flow = modbus_read_registers(ctx, 6, 2, register_value); 
                if (rc_flow == -1) { ; };
            } while (rc_flow == -1); 

            float flow;
            memcpy(&flow, register_value, sizeof(flow)); 
            cout << "Flow = " << flow << " l/m" << endl;

            current_tunening += calculatePID(setPointFlow , flow);
            if (current_tunening < 10) {
                current_tunening = 10;
            }
            if (current_tunening > 40) {
                current_tunening = 40;
                cout << "Blower support just " << current_tunening << endl; 
            }

            cout << "Current PID Tunening = " << current_tunening << endl;
            set_voltage(serial, current_tunening);

            // คำนวณเวลาที่ผ่านไปจากจุดเริ่มต้น
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();

            // บันทึกข้อมูลลงไฟล์
            data_file << flow << "," << elapsed_time << "\n"; 
            data_file.flush(); // บันทึกข้อมูลลงไฟล์ทันที

            cout << "---------------------------------------------------------" << endl;

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

        // ปิดไฟล์เมื่อโปรแกรมถูกหยุด
        data_file.close();
        cout << "File closed. Program exited safely." << endl;
    }
    catch (const std::exception &e)
    {
        cerr << "Error: " << e.what() << endl;
    }

    return 0;
}
