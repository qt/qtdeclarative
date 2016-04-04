HEADERS += \
    $$PWD/qquickbusyindicatorring_p.h \
    $$PWD/qquickprogressstrip_p.h \
    $$PWD/qquickdialring_p.h

SOURCES += \
    $$PWD/qquickbusyindicatorring.cpp \
    $$PWD/qquickprogressstrip.cpp \
    $$PWD/qquickdialring.cpp

QML_CONTROLS = \
    ApplicationWindow.qml \
    BusyIndicator.qml \
    Button.qml \
    CheckBox.qml \
    CheckDelegate.qml \
    CheckIndicator.qml \
    ComboBox.qml \
    Dial.qml \
    Drawer.qml \
    Frame.qml \
    GroupBox.qml \
    ItemDelegate.qml \
    Label.qml \
    Menu.qml \
    MenuItem.qml \
    Page.qml \
    PageIndicator.qml \
    Pane.qml \
    Popup.qml \
    ProgressBar.qml \
    RadioButton.qml \
    RadioDelegate.qml \
    RadioIndicator.qml \
    RangeSlider.qml \
    ScrollBar.qml \
    ScrollIndicator.qml \
    Slider.qml \
    SpinBox.qml \
    StackView.qml \
    SwipeDelegate.qml \
    Switch.qml \
    SwitchIndicator.qml \
    SwitchDelegate.qml \
    SwipeView.qml \
    TabBar.qml \
    TabButton.qml \
    TextArea.qml \
    TextField.qml \
    ToolBar.qml \
    ToolButton.qml \
    ToolTip.qml \
    Tumbler.qml

!qtquickcompiler: QML_FILES += $$QML_CONTROLS
