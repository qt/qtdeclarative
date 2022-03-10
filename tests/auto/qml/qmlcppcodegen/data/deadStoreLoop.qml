import QtQuick
import QtQuick.Controls.Basic

Item {
    Menu {
        id: m
    }
    function c() {
        while (m.count > 0) {
            m.removeItem(m.itemAt(0))
        }
    }
}
