TEMPLATE = app

QT += qml quick
SOURCES += main.cpp

RESOURCES += calqlatr.qrc \
    ../../shared/shared.qrc

OTHER_FILES = calqlatr.qml \
    content/Button.qml \
    content/Display.qml \
    content/NumberPad.qml \
    content/StyleLabel.qml \
    content/audio/touch.wav \
    content/calculator.js \
    content/images/icon-back.png \
    content/images/icon-close.png \
    content/images/icon-settings.png \
    content/images/logo.png \
    content/images/paper-edge-left.png \
    content/images/paper-edge-right.png \
    content/images/paper-grip.png \
    content/images/settings-selected-a.png \
    content/images/settings-selected-b.png \
    content/images/touch-green.png \
    content/images/touch-white.png

target.path = $$[QT_INSTALL_EXAMPLES]/quick/demos/calqlatr
INSTALLS += target
