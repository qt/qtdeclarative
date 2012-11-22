QT += qml

SOURCES += main.cpp \
           person.cpp \
           birthdayparty.cpp \
           happybirthdaysong.cpp 
HEADERS += person.h \
           birthdayparty.h \
           happybirthdaysong.h

RESOURCES += binding.qrc
target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/qml/referenceexamples/binding
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS binding.pro example.qml
sources.path = $$target.path
INSTALLS += target sources
