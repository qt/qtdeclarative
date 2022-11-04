import QtQml 2.15
import QtQml.Models 2.0

ListModel {
    id: self

    // agent was added in 2.14 and should be invisible
    // The QtQml.Models import's qmldir imports should take precedence the QtQml import's
    // since it's more direct.
    // A QtQuick import would be less direct since it adds one more level of indirection.
    property QtObject theAgent: agent
}
