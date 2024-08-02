import QtQuick
import QtQuick.Controls.Basic // the warning "Using attached type Test..." doesn't happen if this is commented out
import AttachedProperties

Window {
    width: Test.value
    height: 600
    visible: true

    Keys.enabled: false
    Item {
        width: Test.value
        Keys.enabled: false

        Item {
            width: Test.value
            Keys.enabled: false

            Item {
                width: Test.value
                Keys.enabled: false
            }
        }
    }
}
