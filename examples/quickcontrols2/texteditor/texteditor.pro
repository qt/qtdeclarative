TEMPLATE = app
TARGET = texteditor
QT += quick quickcontrols2 widgets

HEADERS += \
    documenthandler.h

SOURCES += \
    texteditor.cpp \
    documenthandler.cpp

OTHER_FILES += \
    qml/*.qml

RESOURCES += \
    texteditor.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quickcontrols2/texteditor
INSTALLS += target
