QT += declarative

SOURCES += main.cpp \
           person.cpp \
           birthdayparty.cpp 
HEADERS += person.h \
           birthdayparty.h
RESOURCES += methods.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/declarative/extending/methods
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS methods.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/declarative/extending/methods
INSTALLS += target sources
