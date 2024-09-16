import QtQuick
import Comps as Comps

Comps.OverlayDrawer {
    id: self

    property var dothing

    Component.onCompleted: dothing = handleOpenIcon.dothing

    function dodo() { dothing() }

    function testInstanceOf() : bool {
        return self instanceof Comps.OverlayDrawer
    }
}
