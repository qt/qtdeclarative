QT       = core qmldevtools-private
CONFIG  += console
CONFIG  -= app_bundle
DESTDIR  = $$QT.declarative.bins
SOURCES += main.cpp

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target

