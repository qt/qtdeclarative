import QtQuick

Image {
    id: sourceImage
    objectName: "sourceImage"
    source: "rect.png"

    Image {
        id: grabbedImage
        objectName: "grabbedImage"
    }
}
