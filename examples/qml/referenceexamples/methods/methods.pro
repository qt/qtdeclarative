QT += qml

SOURCES += main.cpp \
           person.cpp \
           birthdayparty.cpp 
HEADERS += person.h \
           birthdayparty.h
RESOURCES += methods.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/qml/referenceexamples/methods
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS methods.pro example.qml
sources.path = $$target.path
INSTALLS += target sources
