import QtQml 2.0
Cycle2 {
    QtObject {} // Having children here has caused qmllint to hang in the past (QTBUG-96343)
}
