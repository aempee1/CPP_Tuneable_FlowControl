cmake_minimum_required(VERSION 3.10)
project(TuneAble)

# กำหนดเวอร์ชัน C++
set(CMAKE_CXX_STANDARD 17)

# กำหนดไดเรกทอรีที่เก็บไฟล์ .cpp และ .hpp
include_directories(./Utils /opt/homebrew/include)

# กำหนดไลบรารีที่ต้องการ
link_directories(/opt/homebrew/lib)

# รวมไฟล์ทั้งหมดในโปรเจค
add_executable(tuneable TuneAble.cpp Utils/serial_utils.cpp Utils/modbus_utils.cpp)

# เชื่อมโยงไลบรารีที่จำเป็น
target_link_libraries(tuneable boost_system modbus)
