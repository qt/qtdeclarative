pragma ComponentBehavior: Bound

import QtQml

QtObject {
    component Inline: QtObject { objectName: "inline" }
    property QtObject o: Inline {}
}
