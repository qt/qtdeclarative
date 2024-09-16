TEMPLATE = app
TARGET = wearable
QT += quick quickcontrols2
qtHaveModule(positioning): QT += positioning
qtHaveModule(location): QT += location

QML_IMPORT_PATH += \
    Wearable \
    WearableSettings \
    WearableStyle

SOURCES += \
    wearable.cpp

RESOURCES += \
    wearable.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quickcontrols/wearable
INSTALLS += target
