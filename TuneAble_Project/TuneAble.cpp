#include <iostream>
#include <boost/asio.hpp>
#include "serial_utils.hpp"
#include "modbus_utils.hpp"
#include <thread>
#include <chrono>

using namespace std ;
using namespace boost::asio ;

const double Kp = 0.04 ; // Kp สูงเกิน = ไม่คงที่ ปรับเร็ว overshoot ง่าย , ต่ำเกิน = ตอบสนองช้า ไม่ถึงเป้าหมาย //0.052
const double Ki = 0.1 ; // Ki สูงเกิน = ไม่คงที่ไม่เสถียร  , ต่ำเกิน = ยังมี error แม้จะเสถียรไปแล้ว //0.04478
const double Kd = 0.0 ; // Kd สูงเกิน = ตอบสนองช้า ลดความไวการปรับตัว , ต่ำเกิน = ไม่คงทีี่่ overshoot มาก //0.082

double integal = 0.0 ;
double previousError = 0.0 ;
double setPointFlow ;

double current_tunening = 10 ;

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

        string response;
        send_scpi_command(serial, "*IDN?", response);
        cout << "IDN Response: " << response << endl;

        cout << "Enter set point Flow (in l/min): ";
        cin >> setPointFlow;

        string response_open;
        send_scpi_command(serial, "OUTPut ON" ,response , false);

        while (true)
        {
            uint16_t register_value[4]; // register save value
            // อ่านค่าจากรีจิสเตอร์ที่อยู่ 6 (flow)
            int rc_flow;
            do
            {
                rc_flow = modbus_read_registers(ctx, 6, 2, register_value); // อ่าน 2 รีจิสเตอร์ที่อยู่ 6
                if (rc_flow == -1){;};
            } while (rc_flow == -1); // ลองใหม่หากเกิด error

            float flow;
            memcpy(&flow, register_value, sizeof(flow)); // แปลงค่าเป็น float
            cout << "Flow = " << flow << " l/m" << endl; // แบ่งด้วย 10 เนื่องจาก resolution เป็น 0.1
            current_tunening  += calculatePID(setPointFlow ,flow);
            if(current_tunening < 10){
                current_tunening = 10;
            }
            if(current_tunening > 40 ){
                current_tunening = 40;
                cout << "Blower support just" << current_tunening << endl; 
            }

            cout << "Current PID Tunening = " << current_tunening << endl; //
            set_voltage(serial,current_tunening);
            cout << "---------------------------------------------------------" <<endl ;
            //----------------------------------------------------------------
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
