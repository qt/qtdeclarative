TEMPLATE = app

QT += quick qml

SOURCES += \
    main.cpp \
    window.cpp \
    engine.cpp

HEADERS += \
    window.h \
    engine.h

RESOURCES += rendercontrol.qrc

LIBS += -ld3d11 -ldxgi -ldxguid -luuid

target.path = $$[QT_INSTALL_EXAMPLES]/quick/rendercontrol/rendercontrol_d3d11
INSTALLS += target
