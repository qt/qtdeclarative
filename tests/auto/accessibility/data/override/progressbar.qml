import QtQuick
import QtQuick.Controls

ProgressBar {
    from: 0
    to: 100
    value: 50

    Accessible.name: "ProgressBarOverride"
}
