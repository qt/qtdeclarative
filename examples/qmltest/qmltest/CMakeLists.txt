# Generated from qmltest.pro.

cmake_minimum_required(VERSION 3.14)
project(tst_qmltestexample LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
#special case begin
find_package(Qt6 COMPONENTS QuickTest)
#special case end

qt_add_executable(tst_qmltestexample
    tst_qmltest.cpp
)
set_target_properties(tst_qmltestexample PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(tst_qmltestexample PUBLIC
    Qt::Core
    Qt::Gui
# special case begin
    Qt::QuickTest
# special case end
)

# special case begin
# This line get added, but does not make sense
#if(QT_BUILDING_QT)
#    target_link_libraries(tst_qmltestexample PUBLIC
#        Qt::Qml
#        Qt::QuickTest
#    )
#endif()
# special case end

if(MACOS AND QT_BUILDING_QT)
    set_target_properties(tst_qmltestexample PROPERTIES
        MACOSX_BUNDLE FALSE
    )
endif()

install(TARGETS tst_qmltestexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
