QT = qml quick qmltest qmldevtools
load(qt_headersclean)
# shadowing problems in scenegraph, allow it for now
*-g++*: QMAKE_CXXFLAGS -= -Wshadow
