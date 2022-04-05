pragma Strict
import QtQuick 6

Item {
    height: !(parent && parent.visible) ? 100 : 0
    width: !(parent && state) ? 100 : 0
}
