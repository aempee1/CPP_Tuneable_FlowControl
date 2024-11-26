cmake_minimum_required(VERSION 3.10)
project(TuneAble)

set(CMAKE_CXX_STANDARD 17)

# ค้นหา Python
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

# กำหนดไดเรกทอรีที่เก็บไฟล์ .cpp และ .hpp
include_directories(./Utils /opt/homebrew/include ./matplotlib-cpp ${Python3_INCLUDE_DIRS})

# กำหนดไลบรารีที่ต้องการ
link_directories(/opt/homebrew/lib)

# รวมไฟล์ทั้งหมดในโปรเจค
add_executable(tuneable TuneAble.cpp Utils/serial_utils.cpp Utils/modbus_utils.cpp)

# เชื่อมโยงไลบรารีที่จำเป็น
target_link_libraries(tuneable boost_system modbus Python3::Python)
