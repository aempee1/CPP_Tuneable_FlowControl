#ifndef MODBUS_UTILS_HPP
#define MODBUS_UTILS_HPP

#include <modbus/modbus.h>

modbus_t* initialize_modbus(const char* device);

#endif // MODBUS_UTILS_HPP
