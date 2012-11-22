QT += qml

SOURCES += main.cpp \
           person.cpp 
HEADERS += person.h
RESOURCES += adding.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/qml/referenceexamples/adding
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS adding.pro example.qml
sources.path = $$target.path
INSTALLS += target sources
