// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qmlls_documentationHints.h"

#include <QtQmlLS/private/qdochtmlparser_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>

tst_qmlls_documentationHints::tst_qmlls_documentationHints()
    : QQmlDataTest(QT_QMLLS_DOCUMENTATION_DATADIR) , m_documentationDataDir(QT_QMLLS_DOCUMENTATION_DATADIR + "/documentationHints"_L1)
{
}

void tst_qmlls_documentationHints::qdochtmlparser_data()
{
    using namespace QQmlJS::Dom;
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<QString>("keyword");
    QTest::addColumn<DomType>("domType");
    QTest::addColumn<HtmlExtractor::ExtractionMode>("extractionMode");
    QTest::addColumn<QString>("expectedDocumentation");

    QTest::addRow("qml-object-type-extended-plaintext")
            << testFile("qdochtmlparser/qml-qtqml-qtobject.html")
            << "QtObject"
            << DomType::QmlObject
            << HtmlExtractor::ExtractionMode::Extended
            << R"(The QtObject type is a non-visual element which contains only the objectName property.
It can be useful to create a QtObject if you need an extremely lightweight type to enclose a set of custom properties:

 import QtQuick

 Item {
     QtObject {
         id: attributes
         property string name
         property int size
         property variant attributes
     }

     Text { text: attributes.name }
 }

It can also be useful for C++ integration, as it is just a plain QObject. See the QObject documentation for further details.)";

    QTest::addRow("qml-object-type-simplified-plaintext")
            << testFile("qdochtmlparser/qml-qtqml-qtobject.html")
            << "QtObject"
            << DomType::QmlObject
            << HtmlExtractor::ExtractionMode::Simplified
            << R"(A basic QML type.)";

    QTest::addRow("qml-property-simplified-plaintext")
            << testFile("qdochtmlparser/qml-qtqml-qtobject.html")
            << "objectName"
            << DomType::PropertyDefinition
            << HtmlExtractor::ExtractionMode::Simplified
            << R"(This property holds the QObject::objectName for this specific object instance.)";

    QTest::addRow("qml-property-simplified-plaintext-from-Qt5")
            << testFile("qdochtmlparser/qml-qtqml-qtobject-qt-5.html")
            << "objectName"
            << DomType::PropertyDefinition
            << HtmlExtractor::ExtractionMode::Simplified
            << R"(This property holds the QObject::objectName for this specific object instance.)";

    QTest::addRow("qml-property-simplified-plaintext")
            << testFile("qdochtmlparser/qml-qtquick-item.html")
            << "width"
            << DomType::PropertyDefinition
            << HtmlExtractor::ExtractionMode::Simplified
            << R"(Defines the item's position and size. The default value is 0.)";

    QTest::addRow("qml-group-property-simplified-plaintext")
            << testFile("qdochtmlparser/qml-qtquick-item.html")
            << "anchors.fill"
            << DomType::PropertyDefinition
            << HtmlExtractor::ExtractionMode::Simplified
            << R"(Anchors provide a way to position an item by specifying its relationship with other items.)";
    QTest::addRow("qml-functions")
            << testFile("qdochtmlparser/qml-qtquick-item.html")
            << "mapFromGlobal"
            << DomType::MethodInfo
            << HtmlExtractor::ExtractionMode::Simplified
            << "Maps the point (x, y), which is in the global coordinate system, to the item's coordinate system,"
            " and returns a point matching the mapped coordinate.";

    QTest::addRow("qml-functions-list")
            << testFile("qdochtmlparser/qml-qtquick-item.html")
            <<  "mapFromItem"
            << DomType::MethodInfo
            << HtmlExtractor::ExtractionMode::Simplified
            << "Maps the point (x, y) or rect (x, y, width, height), which is in item's coordinate system,"
            " to this item's coordinate system, and returns a point or rect matching the mapped coordinate.";
    QTest::addRow("qml-signal")
            << testFile("qdochtmlparser/qml-qtquick-mousearea.html")
            << "pressAndHold"
            << DomType::MethodInfo
            << HtmlExtractor::ExtractionMode::Simplified
            << "This signal is emitted when there is a long press (currently 800ms). The mouse parameter provides information about the press, "
            "including the x and y position of the press, and which button is pressed.";

    // Some properties and methods can be shown as in groups in qt-docs, like width and height of Item.
    QTest::addRow("multiple-entries")
            << testFile("qdochtmlparser/qml-qtquick-mousearea.html")
            << "pressAndHold"
            << DomType::MethodInfo
            << HtmlExtractor::ExtractionMode::Simplified
            << "This signal is emitted when there is a long press (currently 800ms). The mouse parameter provides information about the press, "
            "including the x and y position of the press, and which button is pressed.";
}

void tst_qmlls_documentationHints::qdochtmlparser()
{
    using namespace QQmlJS::Dom;
    QFETCH(QString, filePath);
    QFETCH(QString, keyword);
    QFETCH(DomType, domType);
    QFETCH(HtmlExtractor::ExtractionMode, extractionMode);
    QFETCH(QString, expectedDocumentation);

    const auto htmlCode = [](const QString &testFileName) {
        QFile file(testFileName);
        if (file.open(QIODeviceBase::ReadOnly | QIODevice::Text))
            return QString::fromUtf8(file.readAll());
        return QString{};
    }(filePath);

    ExtractDocumentation extractor(domType);
    const auto actual = extractor.execute(htmlCode, keyword, extractionMode);
    QCOMPARE(actual, expectedDocumentation);
}

void tst_qmlls_documentationHints::skipParsingHtmlTags_data()
{
    using namespace QQmlJS::Dom;
    QTest::addColumn<QString>("rawHtml");
    QTest::addColumn<QString>("expectedText");
    QTest::addColumn<QString>("keyword");
    QTest::addColumn<DomType>("domType");

    QTest::addRow("htmlWithTag")
        << R"("<a name="pressedButtons-prop"></a>
            <div class="qmldoc"><p><b>Note: </b>
            Due to historical reasons, this property is not equivalent to Item.enabled
            It only affects mouse events, and its effect does not propagate to child items.</p>
            <p>This property holds the mouse buttons currently pressed.</p>")"
        << "This property holds the mouse buttons currently pressed."
        << "pressedButtons"
        << DomType::PropertyDefinition;
}

void tst_qmlls_documentationHints::skipParsingHtmlTags()
{
    using namespace QQmlJS::Dom;
    QFETCH(QString, rawHtml);
    QFETCH(QString, expectedText);
    QFETCH(QString, keyword);
    QFETCH(DomType, domType);
    ExtractDocumentation extractor(domType);
    const auto actual = extractor.execute(rawHtml, keyword, HtmlExtractor::ExtractionMode::Simplified);
    QCOMPARE(actual, expectedText);
}

QTEST_MAIN(tst_qmlls_documentationHints)
