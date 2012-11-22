QT += qml

SOURCES += main.cpp \
           person.cpp \
           birthdayparty.cpp 
HEADERS += person.h \
           birthdayparty.h
RESOURCES += attached.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/qml/referenceexamples/attached
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS attached.pro example.qml
sources.path = $$target.path
INSTALLS += target sources
