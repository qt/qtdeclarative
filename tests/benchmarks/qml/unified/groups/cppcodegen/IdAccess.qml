pragma Strict
import QtQuick

Item {
    id: root
    onXChanged: root.y = x
}
