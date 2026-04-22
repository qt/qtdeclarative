import QtQuick
import QtQuick.VectorImage

Rectangle {
    id: topLevelItem
    width: 300
    height: 600
    property bool suspendGrabbing: vi1.status === VectorImage.Loading
                                   || vi1.status === VectorImage.Null
                                   || vi2.status === VectorImage.Loading
                                   || vi2.status === VectorImage.Null

    Column {
        VectorImage {
            id: vi1
            source: "../shared/svg/extended_features/boxGauss.svg"
            asynchronous: false
            width: 300
            height: 300
        }
        VectorImage {
            id: vi2
            source: "../shared/svg/extended_features/boxGauss.svg"
            asynchronous: true
            width: 300
            height: 300
        }
    }
}
