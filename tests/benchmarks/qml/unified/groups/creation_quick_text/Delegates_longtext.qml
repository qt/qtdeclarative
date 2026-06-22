import QtQuick
import QmlBench

// Tests the creation of Text with a long string of latin1 text.
CreationBenchmark {
    id: root;
    count: 20;
    staticCount: 500;
    delegate: Text {
        x: QmlBench.getRandom() * (root.width - width)
        y: QmlBench.getRandom() * (root.height - height)
        width: root.width * 0.33
        wrapMode: Text.WordWrap
        font.pixelSize: 10
        text: "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nunc in volutpat nibh, ut convallis neque. Nam placerat tellus et ante feugiat condimentum. Nulla facilisi. Sed in lobortis magna. Duis eu mattis ante. Donec pulvinar ante est, ac ultricies elit commodo vitae. Sed ut luctus odio, quis aliquet metus. Curabitur quis lacus fringilla, luctus justo id, volutpat dui. Aliquam id nunc gravida, pharetra augue ac, tristique nibh."
    }
}
