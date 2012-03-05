QT += qml

SOURCES += main.cpp \
           person.cpp \
           birthdayparty.cpp 
HEADERS += person.h \
           birthdayparty.h
RESOURCES += default.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/extending/default
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS default.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/extending/default
INSTALLS += target sources
