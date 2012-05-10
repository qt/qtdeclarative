TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/quick/canvas
qml.files = canvas.qml  bezierCurve  clip quadraticCurveTo  roundedrect  smile  squircle  tiger
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/quick/canvas
INSTALLS += target qml