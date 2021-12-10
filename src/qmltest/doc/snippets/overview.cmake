#! [cmake_use]
find_package(Qt6 REQUIRED COMPONENTS QuickTest)
target_link_libraries(mytarget PRIVATE Qt6::QuickTest)
#! [cmake_use]
