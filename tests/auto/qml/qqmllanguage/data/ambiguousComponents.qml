import QtQuick
import Comps as Comps

Comps.OverlayDrawer {
    property var dothing

    Component.onCompleted: dothing = handleOpenIcon.dothing

    function dodo() { dothing() }
}
