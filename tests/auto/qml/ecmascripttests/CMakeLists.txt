# Generated from ecmascripttests.pro.

#####################################################################
## tst_ecmascripttests Test:
#####################################################################

add_qt_test(tst_ecmascripttests
    SOURCES
        qjstest/test262runner.cpp qjstest/test262runner.h
        tst_ecmascripttests.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}\\\"
    PUBLIC_LIBRARIES
        Qt::QmlPrivate
)

#### Keys ignored in scope 2:.:.:testcase.pro:<TRUE>:
# QT_TOOL_ENV = "checkenv"
# checkenv.name = "QTEST_FUNCTION_TIMEOUT"
# checkenv.value = "500000"

## Scopes:
#####################################################################
add_subdirectory(qjstest)
