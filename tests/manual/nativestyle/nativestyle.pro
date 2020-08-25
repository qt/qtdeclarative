QT += quick quickcontrols2

SOURCES += \
        main.cpp

QML_FILES = \
    main.qml \
    ControlContainer.qml \
    Buttons.qml \
    CustomButtons.qml \
    Sliders.qml \
    SlidersSmall.qml \
    SlidersMini.qml \
    CustomSliders.qml \
    CheckBoxes.qml \
    CustomCheckBoxes.qml \
    RadioButtons.qml \
    CustomRadioButtons.qml \
    SpinBoxes.qml \
    CustomSpinBoxes.qml \
    TextFields.qml \
    CustomTextFields.qml \
    Frames.qml \
    CustomFrames.qml \
    TextAreas.qml \
    CustomTextAreas.qml \
    ComboBoxes.qml \
    CustomComboBoxes.qml \
    ScrollBars.qml \
    CustomScrollBars.qml \
    ProgressBars.qml \
    CustomProgressBars.qml \
    Dials.qml \
    CustomDials.qml \

OTHER_FILES += $$QML_FILES
RESOURCES += $$QML_FILES
RESOURCES += checkbox-icon.png checkbox-icon16.png checkbox-icon@2x.png

