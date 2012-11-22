QT += qml

SOURCES += main.cpp \
           person.cpp \
           birthdayparty.cpp 
HEADERS += person.h \
           birthdayparty.h
RESOURCES += properties.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/qml/referenceexamples/properties
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS properties.pro example.qml
sources.path = $$target.path
INSTALLS += target sources
