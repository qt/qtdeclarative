QT += declarative

SOURCES += main.cpp \
           person.cpp \
           birthdayparty.cpp 
HEADERS += person.h \
           birthdayparty.h
RESOURCES += coercion.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/declarative/extending/coercion
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS coercion.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/declarative/extending/coercion
INSTALLS += target sources
