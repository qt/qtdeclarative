import QtQml

QtObject {
    property QtObject target
    property string property
    default property list<Keyframe> keyframes
    property url keyframeSource
}
