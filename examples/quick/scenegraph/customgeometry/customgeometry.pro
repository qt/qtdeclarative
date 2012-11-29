TARGET = customgeometry
QT += quick

SOURCES += \
    main.cpp \
    beziercurve.cpp

HEADERS += \
    beziercurve.h

target.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/customgeometry
qml.files = main.qml
qml.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/customgeometry

INSTALLS += target qml
