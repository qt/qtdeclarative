import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

RowLayout {

    Tumbler {
        model: 4
    }

    Tumbler {
        model: 4
        enabled: false
    }

    Tumbler {
        model: 4
        focus: true
    }

    Row {
        LayoutMirroring.enabled: true
        LayoutMirroring.childrenInherit: true
        Tumbler {
            model: 12
        }

        Tumbler {
            model: ["AM", "PM"]
        }
    }
}
