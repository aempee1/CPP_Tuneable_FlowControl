# CPP_Tuneable_FlowControl

- **prompt** TuneAble : g++ -std=c++17 -o tuneable TuneAble.cpp serial_utils.cpp -I/opt/homebrew/include -L/opt/homebrew/lib -lboost_system -lmodbus

- **prompt**  Read_data_Modbus: g++ -std=c++17 -o init_modbus init_modbus.cpp -I/opt/homebrew/include -L/opt/homebrew/lib -lmodbus

- **prompt** Read_PowerSupply : g++ -std=c++17 -o main main.cpp serial_utils.cpp -I/opt/homebrew/include -L/opt/homebrew/lib -lboost_system