TEMPLATE = app

QT += quick qml

HEADERS += maskedmousearea.h

SOURCES += main.cpp \
           maskedmousearea.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/quick/maskedmousearea
qml.files = maskedmousearea.qml images
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/quick/maskedmousearea
INSTALLS += target qml

