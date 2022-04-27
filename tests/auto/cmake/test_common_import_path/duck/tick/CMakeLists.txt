qt_add_library(duck_tick STATIC)

qt_add_qml_module(duck_tick
    URI duck.tick
    VERSION 1.0
    QML_FILES
      Tick.qml
)

