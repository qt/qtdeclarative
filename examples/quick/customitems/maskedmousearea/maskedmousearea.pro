TEMPLATE = app

QT += quick qml

HEADERS += maskedmousearea.h

SOURCES += main.cpp \
           maskedmousearea.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/customitems/maskedmousearea
qml.files = maskedmousearea.qml images
qml.path = $$target.path
sources.files = $$SOURCES $$HEADERS maskedmousearea.pro
sources.path = $$qml.path
INSTALLS += sources target qml
