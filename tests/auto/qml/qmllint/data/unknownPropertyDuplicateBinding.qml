import QtQuick

Item {
    inner.states: [ // qmllint disable unqualified unresolved-type
      State {name: "foo"}, // qmllint disable missing-property
      State {name: "bar"}  // qmllint disable missing-property
    ]
}
