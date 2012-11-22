TARGET = customgeometry
QT += quick

SOURCES += \
    main.cpp \
    beziercurve.cpp

HEADERS += \
    beziercurve.h

sources.files = $$SOURCES $$HEADERS customgeometry.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/scenegraph/customgeometry
target.path = $$sources.path
INSTALLS += sources target
