import QtQuick

Item {
	width: 300
	height: 300

	Image {
        objectName: "image"
		anchors.fill: parent
		fillMode: Image.PreserveAspectFit
        source: "qt-logo.png"
		layer.enabled: true
	}
}
