import QtQuick
import QtQuick.Controls

SpinBox {
    from: 0
    to: 100
    value: 50
    stepSize: 1
    Accessible.name: "SpinBoxOverride"
}
