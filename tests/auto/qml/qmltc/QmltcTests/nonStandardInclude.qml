import QtQuick
import QmltcTests 1.0

Item {
    NameConflict {
        id: foo
    }
    property bool good: (foo.x == 42)
}
