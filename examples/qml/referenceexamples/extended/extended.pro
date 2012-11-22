QT += qml widgets

SOURCES += main.cpp \
           lineedit.cpp 
HEADERS += lineedit.h
RESOURCES += extended.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/qml/referenceexamples/extended
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS extended.pro example.qml
sources.path = $$target.path
INSTALLS += target sources
