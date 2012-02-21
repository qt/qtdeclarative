QT += declarative

SOURCES += main.cpp \
           person.cpp \
           birthdayparty.cpp 
HEADERS += person.h \
           birthdayparty.h
RESOURCES += signal.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/declarative/extending/signal
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS signal.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/declarative/extending/signal
INSTALLS += target sources
