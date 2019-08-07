# Generated from ecmascripttests.pro.

#####################################################################
## tst_ecmascripttests Test:
#####################################################################

add_qt_test(tst_ecmascripttests
    GUI
    SOURCES
        qjstest/test262runner.cpp qjstest/test262runner.h
        tst_ecmascripttests.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}\\\"
    LIBRARIES
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Qml
)

#### Keys ignored in scope 2:.:.:testcase.pro:<TRUE>:
# CONFIG = "testcase"
# QT_TOOL_ENV = "checkenv"
# checkenv.name = "QTEST_FUNCTION_TIMEOUT"
# checkenv.value = "500000"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:testcase.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
add_subdirectory(qjstest)
