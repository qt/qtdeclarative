import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T

ApplicationWindow {
    width: 600
    height: 400

    Action {
        id: copyAction
        text: "Copy"
    }

    // Based on FluentWinUI3's Menu.
    T.Menu {
        id: menu
        popupType: Popup.Item

        implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                                implicitContentWidth + leftPadding + rightPadding)
        implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                                 implicitContentHeight + topPadding + bottomPadding)

        delegate: MenuItem { }

        contentItem: ListView {
            implicitHeight: contentHeight
            model: menu.contentModel
            interactive: Window.window
                         ? contentHeight + menu.topPadding + menu.bottomPadding > menu.height
                         : false
            currentIndex: menu.currentIndex
            spacing: 4
            clip: true

            ScrollIndicator.vertical: ScrollIndicator {}
        }

        enter: Transition {
            NumberAnimation {
                property: "height"
                from: menu.implicitHeight * 0.33
                to: menu.implicitHeight
                easing.type: Easing.OutCubic
                duration: 1
            }
        }

        background: Rectangle {
            implicitWidth: 200
            implicitHeight: 30
            border.width: 1
        }

        T.Overlay.modal: Rectangle {
            color: "transparent"
        }

        T.Overlay.modeless: Rectangle {
            color: "transparent"
        }

        Action {
            text: "Cut"
        }
    }

    Component.onCompleted: {
        menu.addAction(copyAction)
    }
}
