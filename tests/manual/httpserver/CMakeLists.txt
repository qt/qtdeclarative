# Generated from httpserver.pro.

#####################################################################
## httpserver Binary:
#####################################################################

add_qt_manual_test(httpserver
    SOURCES
        ../../auto/shared/testhttpserver.cpp ../../auto/shared/testhttpserver.h
        main.cpp
    INCLUDE_DIRECTORIES
        ../../auto/shared
    PUBLIC_LIBRARIES
        Qt::Network
)

#### Keys ignored in scope 1:.:.:httpserver.pro:<TRUE>:
# TEMPLATE = "app"
