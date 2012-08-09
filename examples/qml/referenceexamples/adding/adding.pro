QT += qml

SOURCES += main.cpp \
           person.cpp 
HEADERS += person.h
RESOURCES += adding.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/extending/adding
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS adding.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qml/extending/adding
INSTALLS += target sources
