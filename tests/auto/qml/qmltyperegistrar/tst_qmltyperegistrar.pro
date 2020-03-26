TEMPLATE = app

QT = core qml testlib
CONFIG += testcase qmltypes
CONFIG -= debug_and_release_target
macos:CONFIG -= app_bundle

SOURCES += tst_qmltyperegistrar.cpp
HEADERS += \
    hppheader.hpp \
    noextheader \
    tst_qmltyperegistrar.h

QMLTYPES_FILENAME = tst_qmltyperegistrar.qmltypes
QML_FOREIGN_METATYPES += foreign/foreign_metatypes.json
QML_IMPORT_NAME = QmlTypeRegistrarTest
QML_IMPORT_VERSION = 1.0

INCLUDEPATH += foreign
LIBS += -Lforeign -lforeign
