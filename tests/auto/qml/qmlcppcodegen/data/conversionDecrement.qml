pragma Strict
import QtQml

QtObject {
    id: panelGrid
    property var pages: 4
    property int currentPageIndex: 0

    onPagesChanged: {
        if (panelGrid.currentPageIndex === 0) {
            panelGrid.currentPageIndex = panelGrid.pages - 2
        } else if (panelGrid.currentPageIndex === panelGrid.pages - 1) {
            panelGrid.currentPageIndex = 0
        } else {
            panelGrid.currentPageIndex -= 1
        }
    }
}
