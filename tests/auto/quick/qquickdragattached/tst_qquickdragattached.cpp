// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QtTest/QSignalSpy>
#include <QtQuick/qquickwindow.h>
#include <QtQuick/private/qquickdrag_p_p.h>

class tst_QQuickDragAttached: public QObject
{
    Q_OBJECT
private slots:
    void setMimeData_data();
    void setMimeData();

    void startDrag_data();
    void startDrag();
};

void tst_QQuickDragAttached::setMimeData_data()
{
    QTest::addColumn<QVariantMap>("mimeData");
    QTest::addColumn<QStringList>("formats");

    const QImage image(50, 50, QImage::Format_RGB32);

    QTest::addRow("empty") << QVariantMap{} << QStringList{};

    const auto makeMap = [](const QString &mime, const QVariant &value) {
        QVariantMap map;
        map[mime] = value;
        return map;
    };

    QTest::addRow("text/plain, string")
        << makeMap("text/plain", QString("string"))
        << QStringList{"text/plain"};
    QTest::addRow("valid charset")
        << makeMap("text/plain;charset=utf-16", QString("string"))
        << QStringList{"text/plain;charset=utf-16"};
    QTest::addRow("image/png, image")
        << makeMap("image/png", image)
        << QStringList{"image/png"};
    QTest::addRow("text/uri-list, string")
        << makeMap("text/uri-list", QString("https://qt-project.org"))
        << QStringList{"text/uri-list"};
    QTest::addRow("text/uri-list, strings")
        << makeMap("text/uri-list", QStringList{"file://foo", "https://www.test.com"})
        << QStringList{"text/uri-list"};
    QTest::addRow("text/uri-list, url")
        << makeMap("text/uri-list", QVariantList{QVariant(QUrl("file://foo"))})
        << QStringList{"text/uri-list"};
    QTest::addRow("text/uri-list, variant")
        << makeMap("text/uri-list", QVariantList{QVariant("file://foo")})
        << QStringList{"text/uri-list"};
    QTest::addRow("mismatch image")
        << makeMap("image/png", "not an image")
        << QStringList{};
    QTest::addRow("missing charset")
        << makeMap("text/plain;charset=", QString("foo"))
        << QStringList{};
    QTest::addRow("invalid charset")
        << makeMap("text/plain;charset=ugh4", QString("ugh4"))
        << QStringList{};
    QTest::addRow("mismatch compat")
        << makeMap("text/plain", image)
        << QStringList{"text/plain"};
    QTest::addRow("mismatch list")
        << makeMap("text/uri-list", QVariantList{image})
        << QStringList{};
}

void tst_QQuickDragAttached::setMimeData()
{
    QFETCH(QVariantMap, mimeData);

    QQuickDragAttached attached(nullptr);
    QSignalSpy spy(&attached, &QQuickDragAttached::mimeDataChanged);

    attached.setMimeData(mimeData);
    int expectedCount = mimeData.isEmpty() ? 0 : 1;
    QCOMPARE(spy.count(), expectedCount);

    QCOMPARE(attached.mimeData(), mimeData);

    attached.setMimeData(mimeData);
    QCOMPARE(spy.count(), expectedCount);

    expectedCount += mimeData.isEmpty() ? 0 : 1;
    attached.setMimeData({});
    QCOMPARE(spy.count(), expectedCount);
}

void tst_QQuickDragAttached::startDrag_data()
{
    setMimeData_data();
}

void tst_QQuickDragAttached::startDrag()
{
#ifdef QT_BUILD_INTERNAL
    QFETCH(QVariantMap, mimeData);
    QFETCH(QStringList, formats);

    QQuickDragAttached attached(nullptr);
    attached.setMimeData(mimeData);

    auto *d = static_cast<QQuickDragAttachedPrivate *>(QObjectPrivate::get(&attached));

    if (formats.isEmpty()) {
        if (!mimeData.isEmpty())
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*QML QQuickDragAttached:.*"));
        else
            QTest::failOnWarning(QRegularExpression(".*"));
    }
    std::unique_ptr<QMimeData> data(d->createMimeData());
    QVERIFY(data);

    QCOMPARE(data->formats(), formats);
#else
    QSKIP("This test relies on private APIs that are only exported in developer-builds");
#endif
}

QTEST_MAIN(tst_QQuickDragAttached)

#include "tst_qquickdragattached.moc"
