QT += qml

SOURCES += main.cpp \
           person.cpp \
           birthdayparty.cpp 
HEADERS += person.h \
           birthdayparty.h
RESOURCES += signal.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/qml/referenceexamples/signal
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS signal.pro example.qml
sources.path = $$target.path
INSTALLS += target sources
