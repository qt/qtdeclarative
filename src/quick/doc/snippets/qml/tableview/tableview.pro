TEMPLATE = app

QT += qml quick

CONFIG += qmltypes
QML_IMPORT_NAME = TableModel
QML_IMPORT_MAJOR_VERSION = 0

SOURCES += \
    cpp-tablemodel.cpp

HEADERS += \
    cpp-tablemodel.h

RESOURCES += \
    cpp-tablemodel.qml \
    qml-tablemodel.qml \
    reusabledelegate.qml \
    tableviewwithheader.qml \
    tableviewwithprovider.qml
