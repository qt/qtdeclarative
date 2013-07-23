TEMPLATE = app

QT += quick qml
SOURCES += main.cpp
RESOURCES += systemdialogs.qrc ../../shared/shared.qrc

OTHER_FILES += \
    systemdialogs.qml \
    FileDialogs.qml \
    ColorDialogs.qml

target.path = $$[QT_INSTALL_EXAMPLES]/quick/dialogs/systemdialogs
INSTALLS += target
