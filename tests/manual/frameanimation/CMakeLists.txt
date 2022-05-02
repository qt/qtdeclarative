qt_internal_add_manual_test(frameanimation
    GUI
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
set(qml_resource_files
    "main.qml"
)

qt_internal_add_resource(frameanimation "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)
