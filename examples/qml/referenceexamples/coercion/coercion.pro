QT += qml

SOURCES += main.cpp \
           person.cpp \
           birthdayparty.cpp 
HEADERS += person.h \
           birthdayparty.h
RESOURCES += coercion.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/qml/referenceexamples/coercion
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS coercion.pro example.qml
sources.path = $$target.path
INSTALLS += target sources
