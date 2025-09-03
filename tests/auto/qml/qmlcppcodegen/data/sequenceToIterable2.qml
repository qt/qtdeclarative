import QtQml
import TestTypes

QtObject {
    property list<Entry> converted: EntrySource.convertEntries(EntrySource.getEntries());
}
