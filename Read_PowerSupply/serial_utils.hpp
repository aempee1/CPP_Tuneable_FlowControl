#ifndef SERIAL_UTILS_HPP
#define SERIAL_UTILS_HPP

#include <string>
#include <boost/asio.hpp>

void send_scpi_command(boost::asio::serial_port& serial, const std::string& command, std::string& response , bool expect_response = true);
void set_voltage(boost::asio::serial_port& serial, double voltage);
void set_current(boost::asio::serial_port& serial, double current);

#endif // SERIAL_UTILS_HPP