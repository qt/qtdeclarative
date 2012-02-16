QT += qml

SOURCES += main.cpp \
           person.cpp \
           birthdayparty.cpp \
           happybirthdaysong.cpp 
HEADERS += person.h \
           birthdayparty.h \
           happybirthdaysong.h
RESOURCES += valuesource.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/extending/valuesource
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS valuesource.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/extending/valuesource
INSTALLS += target sources
