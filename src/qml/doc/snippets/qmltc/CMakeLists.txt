set(application_name tst_qmltc_examples)
#[[
#! [qmltc-app-name]
# Use "my_qmltc_example" as an application name:
set(application_name my_qmltc_example)
#! [qmltc-app-name]
]]

qt_internal_add_test(${application_name}
    SOURCES
        tst_qmltc_examples.cpp
        colorpicker.h colorpicker.cpp
    LIBRARIES
        Qt::Core
        Qt::Qml
        Qt::Quick
        Qt::QuickTemplates2Private # special
        Qt::Gui
)

#! [qmltc-add-qml-module]
# Create a CMake target, add C++ source files, link libraries, etc...

# Make the application into a proper QML module:
qt6_add_qml_module(${application_name}
    VERSION 1.0
    URI QmltcExample
    QML_FILES
        myApp.qml
        MyButton.qml
        MySlider.qml
)
#! [qmltc-add-qml-module]

#! [qmltc-compile-to-cpp]
# (qmltc-specific) Link *private* libraries that correspond to QML modules:
target_link_libraries(${application_name} PRIVATE Qt::QmlPrivate Qt::QuickPrivate)

# Compile qml files (listed in FILES) to C++ using qmltc and add these files to
# the application binary:
qt6_target_compile_qml_to_cpp(${application_name}
    FILES
        myApp.qml
        MyButton.qml
        MySlider.qml
)
#! [qmltc-compile-to-cpp]
