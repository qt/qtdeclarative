cmake_minimum_required(VERSION 3.16)

project(check_for_existence_of_plugin_targets)

find_package(Qt6 COMPONENTS REQUIRED Qml)

# See QTBUG-43438
if(NOT TARGET Qt6::QTcpServerConnectionFactoryPlugin)
    message(FATAL_ERROR "Qt6::QTcpServerConnectionFactoryPlugin does not exist")
endif()
if(NOT TARGET Qt6::QLocalClientConnectionFactoryPlugin)
    message(FATAL_ERROR "Qt6::QLocalClientConnectionFactoryPlugin does not exist")
endif()
