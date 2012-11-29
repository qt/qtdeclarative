TEMPLATE = app

QT += quick qml

HEADERS += maskedmousearea.h

SOURCES += main.cpp \
           maskedmousearea.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/quick/customitems/maskedmousearea
qml.files = maskedmousearea.qml images
qml.path = $$target.path
INSTALLS += target qml
