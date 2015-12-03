import QtQuick 2.14

BorderImage {
    source: "multi.ico"
    asynchronous: true
    border { left: 19; top: 19; right: 19; bottom: 19 }
    width: 160; height: 160
    horizontalTileMode: BorderImage.Stretch
}
