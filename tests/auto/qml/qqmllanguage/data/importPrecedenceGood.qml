import QtQuick 2.0
import QtQml 2.15

ListModel {
    id: self

    // agent was added in 2.14 and should be visible
    // The QtQml import's qmldir imports should take precedence the QtQuick import's
    property QtObject theAgent: agent
}
