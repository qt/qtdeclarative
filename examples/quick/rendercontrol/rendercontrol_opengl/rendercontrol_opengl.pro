TEMPLATE = app

QT += quick qml opengl

SOURCES += main.cpp \
           window_singlethreaded.cpp \
           cuberenderer.cpp

HEADERS += window_singlethreaded.h \
           cuberenderer.h

RESOURCES += rendercontrol.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/rendercontrol/rendercontrol_opengl
INSTALLS += target
