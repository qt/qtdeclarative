TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/canvas
qml.files = canvas.qml bezierCurve clip quadraticCurveTo roundedrect smile squircle tiger contents
qml.path = $$[QT_INSTALL_EXAMPLES]/qtquick/quick/canvas
INSTALLS += target qml