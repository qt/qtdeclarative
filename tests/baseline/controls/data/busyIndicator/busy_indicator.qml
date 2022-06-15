import QtQuick
import QtQuick.Controls

Column {
    spacing: 2

    BusyIndicator {
    }

    BusyIndicator {
        LayoutMirroring.enabled: true
    }

    BusyIndicator {
        enabled: false
    }

    BusyIndicator {
        running: false
    }
}