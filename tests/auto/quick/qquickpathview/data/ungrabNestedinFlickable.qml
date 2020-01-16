import QtQuick 2.9

Flickable {
    width: 480
    height: 480
    contentX: 0
    contentWidth: width
    contentHeight: height
    leftMargin: 408
    rightMargin: 36
    maximumFlickVelocity: 0
    boundsBehavior: Flickable.StopAtBounds
    flickableDirection: Flickable.HorizontalFlick

    PathView {
        id:pathView
        objectName: "pathView"

        property int countclick: 0

        readonly property int contentsWidth: 348
        readonly property int contentsHeight: 480

        width: contentsWidth
        height: contentsHeight

        interactive: true

        cacheItemCount: 10
        currentIndex: 2
        pathItemCount: 4
        highlightMoveDuration: 300
        highlightRangeMode : PathView.StrictlyEnforceRange
        preferredHighlightBegin: 0.5
        preferredHighlightEnd: 0.5
        snapMode : PathView.SnapOneItem

        path: Path {
            startX: pathView.contentsWidth / 2 - 800
            startY: pathView.contentsHeight / 2 - 800

            PathArc {
                x: pathView.contentsWidth / 2 - 800
                y: pathView.contentsHeight / 2 + 800
                radiusX: 800
                radiusY: 800
                direction: PathArc.Clockwise
            }
        }

        model: ListModel {
            ListElement { objectName:"aqua"; name: "aqua" ;mycolor:"aqua"}
            ListElement { objectName:"blue"; name: "blue" ;mycolor:"blue"}
            ListElement { objectName:"blueviolet"; name: "blueviolet" ;mycolor:"blueviolet"}
            ListElement { objectName:"brown"; name: "brown" ;mycolor:"brown"}
            ListElement { objectName:"chartreuse"; name: "chartreuse" ;mycolor:"chartreuse"}
        }

        delegate: Item {
            id: revolveritem
            objectName: model.objectName

            width: pathView.contentsWidth
            height: pathView.contentsHeight

            Rectangle
            {
                id:myRectangle
                color: mycolor
                width: pathView.contentsWidth -20
                height: pathView.contentsHeight -20
            }
        }
    }
}
