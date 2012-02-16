QT += qml widgets

SOURCES += main.cpp \
           lineedit.cpp 
HEADERS += lineedit.h
RESOURCES += extended.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/extending/extended
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS extended.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/extending/extended
INSTALLS += target sources
