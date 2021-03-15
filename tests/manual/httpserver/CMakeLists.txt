# Generated from httpserver.pro.

#####################################################################
## httpserver Binary:
#####################################################################

qt_internal_add_manual_test(qml_httpserver_manual # special case
    SOURCES
        ../../auto/shared/testhttpserver.cpp ../../auto/shared/testhttpserver.h
        main.cpp
    INCLUDE_DIRECTORIES
        ../../auto/shared
    PUBLIC_LIBRARIES
        Qt::Network
        Qt::Test # special case
        Qt::Qml # special case
)

#### Keys ignored in scope 1:.:.:httpserver.pro:<TRUE>:
# TEMPLATE = "app"
