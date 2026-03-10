import QtQuick
import completions.CppTypes

Item {
	Container { id: container }
	Component.onCompleted: console.log(container.contained.x)
}
