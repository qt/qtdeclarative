pragma Strict
import TestTypes
import QtQuick

Loader {
    id: self
    source: "BaseMember.qml"
    property int ppp: -99

    onItemChanged: {
        var base = item as BaseMember;
        if (base)
            base.ppp = ppp
    }
}
