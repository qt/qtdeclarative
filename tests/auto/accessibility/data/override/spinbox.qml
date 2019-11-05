import QtQuick 2.12
import QtQuick.Controls 2.12

SpinBox {
    from: 0
    to: 100
    value: 50
    stepSize: 1
    Accessible.name: "SpinBoxOverride"
}
