pragma Strict
import QtQuick 6

Item {
    property bool a: !x
    width: !(parent && state) ? 100 : 0

    property font f
    property bool b: !(parent || f)
}
