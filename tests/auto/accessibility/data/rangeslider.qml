import QtQuick 2.5
import QtQuick.Window 2.2
import QtQuick.Controls 2.0

Window {
    visible: true

    RangeSlider {
        id: rangeSlider
        objectName: "rangeslider"
        from: 0
        to: 100
        first.value: 25
        second.value: 75
        stepSize: 1
        orientation: "Horizontal"
    }
}
