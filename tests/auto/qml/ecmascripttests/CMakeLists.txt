#####################################################################
## tst_ecmascripttests Test:
#####################################################################

qt_internal_add_test(tst_ecmascripttests
    SOURCES
        qjstest/test262runner.cpp qjstest/test262runner.h
        tst_ecmascripttests.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}\\\"
    PUBLIC_LIBRARIES
        Qt::QmlPrivate
)

set_property(TEST tst_ecmascripttests APPEND PROPERTY ENVIRONMENT "QTEST_FUNCTION_TIMEOUT=900000")
if(NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(qjstest)
endif()
