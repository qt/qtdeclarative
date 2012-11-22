QT += qml

SOURCES += main.cpp \
           person.cpp \
           birthdayparty.cpp 
HEADERS += person.h \
           birthdayparty.h
RESOURCES += grouped.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/qml/referenceexamples/grouped
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS grouped.pro example.qml
sources.path = $$target.path
INSTALLS += target sources
