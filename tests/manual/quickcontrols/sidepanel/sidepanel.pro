TEMPLATE = app
TARGET = sidepanel
QT += quick

SOURCES += \
    sidepanel.cpp

RESOURCES += \
    doc/images/qtquickcontrols-sidepanel-landscape.png \
    doc/images/qtquickcontrols-sidepanel-portrait.png \
    images/qt-logo@2x.png \
    images/qt-logo@3x.png \
    images/qt-logo@4x.png \
    images/qt-logo.png \
    qtquickcontrols2.conf \
    sidepanel.qml

target.path = $$[QT_INSTALL_EXAMPLES]/quickcontrols/sidepanel
INSTALLS += target
