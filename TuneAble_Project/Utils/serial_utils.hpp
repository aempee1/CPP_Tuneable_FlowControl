#ifndef SERIAL_UTILS_HPP
#define SERIAL_UTILS_HPP

#include <string>
#include <boost/asio.hpp>
#include <vector>

void send_scpi_command(boost::asio::serial_port& serial, const std::string& command, std::string& response , bool expect_response = true);
void set_voltage(boost::asio::serial_port& serial, double voltage);
void set_current(boost::asio::serial_port& serial, double current);

std::vector<std::string> list_ports();
int select_port(const std::vector<std::string>& available_ports);
boost::asio::serial_port init_serial_port(boost::asio::io_service& io, const std::string& port_name);

#endif // SERIAL_UTILS_HPP