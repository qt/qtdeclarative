import QtQml

QtObject {
    property real startFrame
    property real endFrame
    property real currentFrame
    default property list<KeyframeGroup> keyframeGroupes
    property list<TimelineAnimation> animations
    property bool enabled
}
