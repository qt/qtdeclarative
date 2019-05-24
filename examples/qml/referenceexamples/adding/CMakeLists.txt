# Generated from adding.pro.

#####################################################################
## adding Binary:
#####################################################################

add_qt_executable(adding
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/qml/referenceexamples/adding"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/qml/referenceexamples/adding"
    SOURCES
        main.cpp
        person.cpp person.h
    PUBLIC_LIBRARIES
        Qt::Qml
)

# Resources:
add_qt_resource(adding "adding" FILES
    example.qml)

