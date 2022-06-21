import QtQuick
import QtQuick.Controls

Item {
    id: delegate
    property bool tumbler: Tumbler.displacement // Read-only
    QtObject {
        property bool tumbler2: delegate.Tumbler.displacement
    }
    QtObject {
        Component.onCompleted: {
            delegate.Accessible.name = "Foo"
        }
    }
}
