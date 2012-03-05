TEMPLATE = app

QT += quick qml
SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qtquick/animation
qml.files = animation.qml  basics  behaviors  easing pathanimation  pathinterpolator  states
qml.path = $$[QT_INSTALL_EXAMPLES]/qtdeclarative/qtquick/animation
INSTALLS += target qml

