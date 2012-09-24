QT += qml

SOURCES += main.cpp \
           person.cpp \
           birthdayparty.cpp \
           happybirthdaysong.cpp 
HEADERS += person.h \
           birthdayparty.h \
           happybirthdaysong.h

RESOURCES += binding.qrc
target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/qml/extending/binding
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS binding.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtquick/qml/extending/binding
INSTALLS += target sources
