import qqmllanguage.Test
import QtQml

VariantAssociationProvider {
    property string emailBeforeChange: ""
    property string emailAfterChange: ""

    Component.onCompleted: {
        const items = getListOfMap();
        emailBeforeChange = items[1].email

        items[1].email = "This Email Should Mutate"
        emailAfterChange = items[1].email
    }
}
