import QtQml

// Overrides the base property "a". The override records the *index* of the base's "a"
// (overrideIndex), i.e. a reference that points into the parent's index space.
OverrideBase {
    property int a: 5
}
