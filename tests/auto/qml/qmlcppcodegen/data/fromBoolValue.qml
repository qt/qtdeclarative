pragma Strict
import QtQuick 6

Item {
    property bool a: !x
    width: !(parent && state) ? 100 : 0
}
