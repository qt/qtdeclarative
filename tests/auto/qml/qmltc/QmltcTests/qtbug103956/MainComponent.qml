import QtQuick
import QmltcTests 1.0

Item {
    property alias firstComponent: firstComponent
    SubComponent {
        id: firstComponent
    }
}
