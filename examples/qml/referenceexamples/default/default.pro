QT += qml

SOURCES += main.cpp \
           person.cpp \
           birthdayparty.cpp 
HEADERS += person.h \
           birthdayparty.h
RESOURCES += default.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/qml/referenceexamples/default
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS default.pro example.qml
sources.path = $$target.path
INSTALLS += target sources
