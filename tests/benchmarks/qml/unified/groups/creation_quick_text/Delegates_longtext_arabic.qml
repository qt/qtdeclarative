import QtQuick
import QmlBench

// Tests the creation of Text with a long string of arabic text.
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
        text: "عل الدمج والمانيا حين. تحت سقوط انذار ان. ودول الخارجية و كلّ. جمعت والتي الساحل في ومن. أن لها أدوات مهمّات الأبرياء. شاسعة المحيط الساحلية ومن قد, فعل إختار العصبة التخطيط ان, هو فسقط موالية ولم. عرض و ببعض كردة إعلان, بين ثم سكان واستمرت المؤلّفة. لم مكن ألمّ اتفاق نتيجة, هُزم الجديدة، إيو بـ, به، عن انذار المارق الآلاف. اتفاق وإعلان بريطانيا، ان بحث, بعد ٣٠ قبضتهم الغالي الجنود. فصل تم وقام جسيمة, يعبأ وبدون الموسوعة ان تعد. ٢٠٠٤ وتتحمّل التخطيط وفي عل."
    }
}

