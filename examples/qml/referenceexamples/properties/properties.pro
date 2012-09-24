QT += qml

SOURCES += main.cpp \
           person.cpp \
           birthdayparty.cpp 
HEADERS += person.h \
           birthdayparty.h
RESOURCES += properties.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/qml/extending/properties
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS properties.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtquick/qml/extending/properties
INSTALLS += target sources
