// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <private/qqmlengine_p.h>

#include <qtest.h>

#include <QQmlEngine>
#include <QQmlComponent>
#include <QtQuick/QQuickItem>

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QSignalSpy>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <QTimeZone>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <private/qtenvironmentvariables_p.h> // for qTzSet()

class tst_qqmlqt : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlqt() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void initTestCase() override;
    void enums();
    void rgba();
    void hsla();
    void hsva();
    void colorEqual();
    void rect();
    void point();
    void size();
    void vector2d();
    void vector3d();
    void vector4d();
    void quaternion();
    void matrix4x4();
    void font();
    void lighter();
    void darker();
    void alpha();
    void tint();
    void color();
    void openUrlExternally();
    void openUrlExternally_pragmaLibrary();
    void md5();
    void createComponent();
    void createComponent_pragmaLibrary();
    void createQmlObject();
    void dateTimeConversion();
    void dateTimeFormatting();
    void dateTimeFormatting_data();
    void dateTimeFormattingVariants();
    void dateTimeFormattingVariants_data();
    void dateTimeFormattingWithLocale();
    void isQtObject();
    void btoa();
    void atob();
    void fontFamilies();
    void quit();
    void exit();
    void resolvedUrl();
    void later_data();
    void later();
    void qtObjectContents();

    void timeRoundtrip_data();
    void timeRoundtrip();

private:
    QQmlEngine engine;
};

// for callLater()
class TestElement : public QQuickItem
{
    Q_OBJECT
public:
    TestElement() : m_intptr(new int) {}
    ~TestElement() { delete m_intptr; }

    Q_INVOKABLE void dangerousFunction() {
        delete m_intptr;
        m_intptr = new int;
        *m_intptr = 5;
    }
private:
    int *m_intptr;
};

// for callLater()
class TestModuleApi : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int intProp READ intProp WRITE setIntProp NOTIFY intPropChanged)

public:
    TestModuleApi() : m_int(0) {}
    ~TestModuleApi() {}

    int intProp() const { return m_int; }
    void setIntProp(int v) { m_int = v; emit intPropChanged(); }

    Q_INVOKABLE void testFunc() { ++m_int; emit intPropChanged(); }
    Q_INVOKABLE void resetIntProp() { m_int = 0; emit intPropChanged(); }

signals:
    void intPropChanged();

private:
    int m_int;
};

static QObject *test_module_api_factory(QQmlEngine *engine, QJSEngine *scriptEngine)
{
   Q_UNUSED(engine);
   Q_UNUSED(scriptEngine);
   TestModuleApi *api = new TestModuleApi;
   return api;
}

void tst_qqmlqt::initTestCase()
{
    QQmlDataTest::initTestCase();
    qmlRegisterSingletonType<TestModuleApi>("LaterImports", 1, 0, "SingletonType", test_module_api_factory);
    qmlRegisterType<TestElement>("LaterImports", 1, 0, "TestElement");
}

void tst_qqmlqt::enums()
{
    QQmlComponent component(&engine, testFileUrl("enums.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(object->property("test1").toInt(), (int)Qt::Key_Escape);
    QCOMPARE(object->property("test2").toInt(), (int)Qt::DescendingOrder);
    QCOMPARE(object->property("test3").toInt(), (int)Qt::ElideMiddle);
    QCOMPARE(object->property("test4").toInt(), (int)Qt::AlignRight);
}

void tst_qqmlqt::rgba()
{
    QQmlComponent component(&engine, testFileUrl("rgba.qml"));

    QString warning1 = "rgba.qml:6: Error: Unable to determine callable overload";
    QString warning2 = component.url().toString() + ":7: Error: Too many arguments";
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);


    QCOMPARE(qvariant_cast<QColor>(object->property("test1")), QColor::fromRgbF(1, 0, 0, 0.8f));
    QCOMPARE(qvariant_cast<QColor>(object->property("test2")), QColor::fromRgbF(1, 0.5f, 0.3f, 1));
    QCOMPARE(qvariant_cast<QColor>(object->property("test3")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test4")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test5")), QColor::fromRgbF(1, 1, 1, 1));
    QCOMPARE(qvariant_cast<QColor>(object->property("test6")), QColor::fromRgbF(0, 0, 0, 0));
}

void tst_qqmlqt::hsla()
{
    QQmlComponent component(&engine, testFileUrl("hsla.qml"));

    QString warning1 = "hsla.qml:6: Error: Unable to determine callable overload";
    QString warning2 = component.url().toString() + ":7: Error: Too many arguments";
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(qvariant_cast<QColor>(object->property("test1")), QColor::fromHslF(1, 0, 0, 0.8f));
    QCOMPARE(qvariant_cast<QColor>(object->property("test2")), QColor::fromHslF(1, 0.5f, 0.3f, 1));
    QCOMPARE(qvariant_cast<QColor>(object->property("test3")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test4")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test5")), QColor::fromHslF(1, 1, 1, 1));
    QCOMPARE(qvariant_cast<QColor>(object->property("test6")), QColor::fromHslF(0, 0, 0, 0));
}

void tst_qqmlqt::hsva()
{
    QQmlComponent component(&engine, testFileUrl("hsva.qml"));

    QString warning1 = "hsva.qml:6: Error: Unable to determine callable overload";
    QString warning2 = component.url().toString() + ":7: Error: Too many arguments";
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(qvariant_cast<QColor>(object->property("test1")), QColor::fromHsvF(1, 0, 0, 0.8f));
    QCOMPARE(qvariant_cast<QColor>(object->property("test2")), QColor::fromHsvF(1, 0.5f, 0.3f, 1));
    QCOMPARE(qvariant_cast<QColor>(object->property("test3")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test4")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test5")), QColor::fromHsvF(1, 1, 1, 1));
    QCOMPARE(qvariant_cast<QColor>(object->property("test6")), QColor::fromHsvF(0, 0, 0, 0));
}

void tst_qqmlqt::colorEqual()
{
    QQmlComponent component(&engine, testFileUrl("colorEqual.qml"));

    QTest::ignoreMessage(QtWarningMsg, qPrintable(component.url().toString() + ":6: Error: Insufficient arguments"));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(component.url().toString() + ":7: Error: Insufficient arguments"));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(component.url().toString() + ":9: Error: Qt.colorEqual(): Invalid color name"));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(component.url().toString() + ":10: Error: Qt.colorEqual(): Invalid color name"));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(component.url().toString() + ":12: Error: Qt.colorEqual(): Invalid arguments"));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(component.url().toString() + ":13: Error: Qt.colorEqual(): Invalid arguments"));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(component.url().toString() + ":17: Error: Qt.colorEqual(): Invalid arguments"));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(component.url().toString() + ":18: Error: Qt.colorEqual(): Invalid arguments"));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(component.url().toString() + ":34: Error: Qt.colorEqual(): Invalid color name"));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(component.url().toString() + ":35: Error: Qt.colorEqual(): Invalid color name"));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(object->property("test1a").toBool(), false);
    QCOMPARE(object->property("test1b").toBool(), false);
    QCOMPARE(object->property("test1c").toBool(), false);
    QCOMPARE(object->property("test1d").toBool(), false);
    QCOMPARE(object->property("test1e").toBool(), false);
    QCOMPARE(object->property("test1f").toBool(), false);
    QCOMPARE(object->property("test1g").toBool(), false);
    QCOMPARE(object->property("test1h").toBool(), false);

    QCOMPARE(object->property("test2a").toBool(), true);
    QCOMPARE(object->property("test2b").toBool(), true);
    QCOMPARE(object->property("test2c").toBool(), true);
    QCOMPARE(object->property("test2d").toBool(), true);
    QCOMPARE(object->property("test2e").toBool(), true);
    QCOMPARE(object->property("test2f").toBool(), true);
    QCOMPARE(object->property("test2g").toBool(), true);
    QCOMPARE(object->property("test2h").toBool(), true);
    QCOMPARE(object->property("test2i").toBool(), false);
    QCOMPARE(object->property("test2j").toBool(), false);
    QCOMPARE(object->property("test2k").toBool(), false);
    QCOMPARE(object->property("test2l").toBool(), false);
    QCOMPARE(object->property("test2m").toBool(), false);
    QCOMPARE(object->property("test2n").toBool(), false);

    QCOMPARE(object->property("test3a").toBool(), true);
    QCOMPARE(object->property("test3b").toBool(), true);
    QCOMPARE(object->property("test3c").toBool(), true);
    QCOMPARE(object->property("test3d").toBool(), true);
    QCOMPARE(object->property("test3e").toBool(), true);
    QCOMPARE(object->property("test3f").toBool(), true);
    QCOMPARE(object->property("test3g").toBool(), false);
    QCOMPARE(object->property("test3h").toBool(), false);
    QCOMPARE(object->property("test3i").toBool(), true);
    QCOMPARE(object->property("test3j").toBool(), true);
    QCOMPARE(object->property("test3k").toBool(), true);
    QCOMPARE(object->property("test3l").toBool(), true);
    QCOMPARE(object->property("test3m").toBool(), true);
    QCOMPARE(object->property("test3n").toBool(), true);

    QCOMPARE(object->property("test4a").toBool(), true);
    QCOMPARE(object->property("test4b").toBool(), true);
    QCOMPARE(object->property("test4c").toBool(), false);
    QCOMPARE(object->property("test4d").toBool(), false);
    QCOMPARE(object->property("test4e").toBool(), false);
    QCOMPARE(object->property("test4f").toBool(), false);
    QCOMPARE(object->property("test4g").toBool(), false);
    QCOMPARE(object->property("test4h").toBool(), false);
    QCOMPARE(object->property("test4i").toBool(), false);
    QCOMPARE(object->property("test4j").toBool(), false);

    QCOMPARE(object->property("test5a").toBool(), true);
    QCOMPARE(object->property("test5b").toBool(), true);
    QCOMPARE(object->property("test5c").toBool(), true);
    QCOMPARE(object->property("test5d").toBool(), false);
    QCOMPARE(object->property("test5e").toBool(), false);

    QCOMPARE(object->property("test6a").toBool(), true);
    QCOMPARE(object->property("test6b").toBool(), true);
    QCOMPARE(object->property("test6c").toBool(), true);
    QCOMPARE(object->property("test6d").toBool(), false);
    QCOMPARE(object->property("test6e").toBool(), false);
}

void tst_qqmlqt::rect()
{
    QQmlComponent component(&engine, testFileUrl("rect.qml"));

    QString warning1 = component.url().toString() + ":6: Error: Insufficient arguments";
    QString warning2 = component.url().toString() + ":7: Error: Too many arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(qvariant_cast<QRectF>(object->property("test1")), QRectF(10, 13, 100, 109));
    QCOMPARE(qvariant_cast<QRectF>(object->property("test2")), QRectF(-10, 13, 100, 109.6));
    QCOMPARE(qvariant_cast<QRectF>(object->property("test3")), QRectF());
    QCOMPARE(qvariant_cast<QRectF>(object->property("test4")), QRectF());
    QCOMPARE(qvariant_cast<QRectF>(object->property("test5")), QRectF(10, 13, 100, -109));
}

void tst_qqmlqt::point()
{
    QQmlComponent component(&engine, testFileUrl("point.qml"));

    QString warning1 = component.url().toString() + ":6: Error: Insufficient arguments";
    QString warning2 = component.url().toString() + ":7: Error: Too many arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(qvariant_cast<QPointF>(object->property("test1")), QPointF(19, 34));
    QCOMPARE(qvariant_cast<QPointF>(object->property("test2")), QPointF(-3, 109.2));
    QCOMPARE(qvariant_cast<QPointF>(object->property("test3")), QPointF());
    QCOMPARE(qvariant_cast<QPointF>(object->property("test4")), QPointF());
}

void tst_qqmlqt::size()
{
    QQmlComponent component(&engine, testFileUrl("size.qml"));

    QString warning1 = component.url().toString() + ":7: Error: Insufficient arguments";
    QString warning2 = component.url().toString() + ":8: Error: Too many arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(qvariant_cast<QSizeF>(object->property("test1")), QSizeF(19, 34));
    QCOMPARE(qvariant_cast<QSizeF>(object->property("test2")), QSizeF(3, 109.2));
    QCOMPARE(qvariant_cast<QSizeF>(object->property("test3")), QSizeF(-3, 10));
    QCOMPARE(qvariant_cast<QSizeF>(object->property("test4")), QSizeF());
    QCOMPARE(qvariant_cast<QSizeF>(object->property("test5")), QSizeF());
}

void tst_qqmlqt::vector2d()
{
    QQmlComponent component(&engine, testFileUrl("vector2.qml"));

    QString warning1 = component.url().toString() + ":6: Error: Insufficient arguments";
    QString warning2 = component.url().toString() + ":7: Error: Too many arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(qvariant_cast<QVector2D>(object->property("test1")), QVector2D(1, 0.9f));
    QCOMPARE(qvariant_cast<QVector2D>(object->property("test2")), QVector2D(102, -982.1f));
    QCOMPARE(qvariant_cast<QVector2D>(object->property("test3")), QVector2D());
    QCOMPARE(qvariant_cast<QVector2D>(object->property("test4")), QVector2D());
}

void tst_qqmlqt::vector3d()
{
    QQmlComponent component(&engine, testFileUrl("vector.qml"));

    QString warning1 = component.url().toString() + ":6: Error: Insufficient arguments";
    QString warning2 = component.url().toString() + ":7: Error: Too many arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(qvariant_cast<QVector3D>(object->property("test1")), QVector3D(1, 0, 0.9f));
    QCOMPARE(qvariant_cast<QVector3D>(object->property("test2")), QVector3D(102, -10, -982.1f));
    QCOMPARE(qvariant_cast<QVector3D>(object->property("test3")), QVector3D());
    QCOMPARE(qvariant_cast<QVector3D>(object->property("test4")), QVector3D());
}

void tst_qqmlqt::vector4d()
{
    QQmlComponent component(&engine, testFileUrl("vector4.qml"));

    QString warning1 = component.url().toString() + ":6: Error: Insufficient arguments";
    QString warning2 = component.url().toString() + ":7: Error: Too many arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(qvariant_cast<QVector4D>(object->property("test1")), QVector4D(1, 0, 0.9f, 0.6f));
    QCOMPARE(qvariant_cast<QVector4D>(object->property("test2")), QVector4D(102, -10, -982.1f, 10));
    QCOMPARE(qvariant_cast<QVector4D>(object->property("test3")), QVector4D());
    QCOMPARE(qvariant_cast<QVector4D>(object->property("test4")), QVector4D());
}

void tst_qqmlqt::quaternion()
{
    QQmlComponent component(&engine, testFileUrl("quaternion.qml"));

    QString warning1 = component.url().toString() + ":6: Error: Insufficient arguments";
    QString warning2 = component.url().toString() + ":7: Error: Too many arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(qvariant_cast<QQuaternion>(object->property("test1")), QQuaternion(2, 17, 0.9f, 0.6f));
    QCOMPARE(qvariant_cast<QQuaternion>(object->property("test2")), QQuaternion(102, -10, -982.1f, 10));
    QCOMPARE(qvariant_cast<QQuaternion>(object->property("test3")), QQuaternion());
    QCOMPARE(qvariant_cast<QQuaternion>(object->property("test4")), QQuaternion());
}

void tst_qqmlqt::matrix4x4()
{
    QQmlComponent component(&engine, testFileUrl("matrix4x4.qml"));

    QString warning1 = component.url().toString() + ":6: Error: Too many arguments";
    QString warning2 = component.url().toString() + ":7: Error: Qt.matrix4x4(): Invalid argument: not a valid matrix4x4 values array";
    QString warning3 = component.url().toString() + ":8: Error: Qt.matrix4x4(): Invalid argument: not a valid matrix4x4 values array";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning3));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(qvariant_cast<QMatrix4x4>(object->property("test1")), QMatrix4x4(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
    QCOMPARE(qvariant_cast<QMatrix4x4>(object->property("test2")), QMatrix4x4(1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4));
    QCOMPARE(qvariant_cast<QMatrix4x4>(object->property("test3")), QMatrix4x4());
    QCOMPARE(qvariant_cast<QMatrix4x4>(object->property("test4")), QMatrix4x4());
    QCOMPARE(qvariant_cast<QMatrix4x4>(object->property("test5")), QMatrix4x4());
}

void tst_qqmlqt::font()
{
    QQmlComponent component(&engine, testFileUrl("font.qml"));

    QString warning1 = component.url().toString() + ":6: Error: Too many arguments";
    QString warning2 = component.url().toString() + ":7: Error: Qt.font(): Invalid argument: no valid font subproperties specified";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QFont f;
    f.setFamily("Arial");
    f.setPointSize(22);
    QCOMPARE(qvariant_cast<QFont>(object->property("test1")), f);
    f.setPointSize(20);
    f.setWeight(QFont::DemiBold);
    f.setItalic(true);
    QCOMPARE(qvariant_cast<QFont>(object->property("test2")), f);
    QCOMPARE(qvariant_cast<QFont>(object->property("test3")), QFont());
    QCOMPARE(qvariant_cast<QFont>(object->property("test4")), QFont());
}

void tst_qqmlqt::lighter()
{
    QQmlComponent component(&engine, testFileUrl("lighter.qml"));

    QString warning1 = "lighter.qml:5: Error: Unable to determine callable overload";
    QString warning2 = component.url().toString() + ":10: Error: Too many arguments";
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(qvariant_cast<QColor>(object->property("test1")), QColor::fromRgbF(1, 0.8f, 0.3f).lighter());
    QCOMPARE(qvariant_cast<QColor>(object->property("testColor1")),
             QColor::fromRgbF(1, 0.8f, 0.3f).lighter());
    QCOMPARE(qvariant_cast<QColor>(object->property("test2")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test3")), QColor::fromRgbF(1, 0.8f, 0.3f).lighter(180));
    QCOMPARE(qvariant_cast<QColor>(object->property("testColor3")),
             QColor::fromRgbF(1, 0.8f, 0.3f).lighter(180));
    QCOMPARE(qvariant_cast<QColor>(object->property("test4")), QColor("red").lighter());
    QCOMPARE(qvariant_cast<QColor>(object->property("testColor4")), QColor("red").lighter());
    QCOMPARE(qvariant_cast<QColor>(object->property("test5")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test6")), QColor());
}

void tst_qqmlqt::darker()
{
    QQmlComponent component(&engine, testFileUrl("darker.qml"));

    QString warning1 = "darker.qml:5: Error: Unable to determine callable overload";
    QString warning2 = component.url().toString() + ":10: Error: Too many arguments";
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(qvariant_cast<QColor>(object->property("test1")), QColor::fromRgbF(1, 0.8f, 0.3f).darker());
    QCOMPARE(qvariant_cast<QColor>(object->property("testColor1")),
             QColor::fromRgbF(1, 0.8f, 0.3f).darker());
    QCOMPARE(qvariant_cast<QColor>(object->property("test2")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test3")), QColor::fromRgbF(1, 0.8f, 0.3f).darker(280));
    QCOMPARE(qvariant_cast<QColor>(object->property("testColor3")),
             QColor::fromRgbF(1, 0.8f, 0.3f).darker(280));
    QCOMPARE(qvariant_cast<QColor>(object->property("test4")), QColor("red").darker());
    QCOMPARE(qvariant_cast<QColor>(object->property("testColor4")), QColor("red").darker());
    QCOMPARE(qvariant_cast<QColor>(object->property("test5")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test6")), QColor());
}

void tst_qqmlqt::alpha()
{
    QQmlComponent component(&engine, testFileUrl("alpha.qml"));

    QString warning1 = component.url().toString() + ":5: Error: Insufficient arguments";
    QString warning2 = component.url().toString() + ":10: Error: Too many arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(qvariant_cast<QColor>(object->property("test1")),
             QColor::fromRgbF(1.0f, 0.8f, 0.3f, 0.5f));
    QCOMPARE(qvariant_cast<QColor>(object->property("testColor1")),
             QColor::fromRgbF(1, 0.8f, 0.3f, 0.5));
    QCOMPARE(qvariant_cast<QColor>(object->property("test2")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test3")),
             QColor::fromRgbF(1.0f, 0.8f, 0.3f, 0.7f));
    QCOMPARE(qvariant_cast<QColor>(object->property("testColor3")),
             QColor::fromRgbF(1, 0.8f, 0.3f, 0.7f));

    QColor alphaRed = QColor("red");
    alphaRed.setAlphaF(0.5f);

    QCOMPARE(qvariant_cast<QColor>(object->property("test4")), alphaRed);
    QCOMPARE(qvariant_cast<QColor>(object->property("testColor4")), alphaRed);
    QCOMPARE(qvariant_cast<QColor>(object->property("test5")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test6")), QColor());
}

void tst_qqmlqt::tint()
{
    QQmlComponent component(&engine, testFileUrl("tint.qml"));

    QString warning1 = component.url().toString() + ":7: Error: Too many arguments";
    QString warning2 = component.url().toString() + ":8: Error: Insufficient arguments";
    QString warning3 = component.url().toString() + ":13: Error: Insufficient arguments";

    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning3));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(qvariant_cast<QColor>(object->property("test1")), QColor::fromRgbF(0, 0, 1));
    QCOMPARE(qvariant_cast<QColor>(object->property("testColor1")), QColor::fromRgbF(0, 0, 1));
    QCOMPARE(qvariant_cast<QColor>(object->property("test2")), QColor::fromRgbF(1, 0, 0));
    QCOMPARE(qvariant_cast<QColor>(object->property("testColor2")), QColor::fromRgbF(1, 0, 0));
    QColor test3 = qvariant_cast<QColor>(object->property("test3"));
    QColor testColor3 = qvariant_cast<QColor>(object->property("testColor3"));
    QCOMPARE(test3.rgba(), 0xFF7F0080);
    QCOMPARE(testColor3.rgba(), 0xFF7F0080);
    QCOMPARE(qvariant_cast<QColor>(object->property("test4")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test5")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("testColor5")), QColor());
}

void tst_qqmlqt::color()
{
    QQmlComponent component(&engine, testFileUrl("color.qml"));

    QStringList warnings = { ":6: Error: \"taint\" is not a valid color name",
                             ":7: Error: \"0.5\" is not a valid color name",
                             ":8: Error: Insufficient arguments",
                             ":9: Error: Too many arguments" };

    for (const QString &warning : warnings)
        QTest::ignoreMessage(QtWarningMsg, qPrintable(component.url().toString() + warning));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(qvariant_cast<QColor>(object->property("test1")), QColor("red"));
    QCOMPARE(qvariant_cast<QColor>(object->property("test2")), QColor("#ff00ff00"));
    QCOMPARE(qvariant_cast<QColor>(object->property("test3")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test4")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test5")), QColor());
    QCOMPARE(qvariant_cast<QColor>(object->property("test6")), QColor());
}

class MyUrlHandler : public QObject
{
    Q_OBJECT
public:
    MyUrlHandler() : called(0) { }
    int called;
    QUrl last;

public slots:
    void noteCall(const QUrl &url) { called++; last = url; }
};

void tst_qqmlqt::openUrlExternally()
{
    MyUrlHandler handler;

    const QUrl htmlTestFile = testFileUrl("test.html");
    QDesktopServices::setUrlHandler("test", &handler, "noteCall");
    QDesktopServices::setUrlHandler(htmlTestFile.scheme(), &handler, "noteCall");
    const auto unset = qScopeGuard([&] {
        QDesktopServices::unsetUrlHandler(htmlTestFile.scheme());
        QDesktopServices::unsetUrlHandler("test");
    });
    QQmlComponent component(&engine, testFileUrl("openUrlExternally.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QCOMPARE(handler.called,1);
    QCOMPARE(handler.last, QUrl("test:url"));

    object->setProperty("testFile", true);

    QCOMPARE(handler.called,2);
    QCOMPARE(handler.last, htmlTestFile);
}

void tst_qqmlqt::openUrlExternally_pragmaLibrary()
{
    MyUrlHandler handler;

    const QUrl htmlTestFile = testFileUrl("test.html");
    QDesktopServices::setUrlHandler("test", &handler, "noteCall");
    QDesktopServices::setUrlHandler(htmlTestFile.scheme(), &handler, "noteCall");
    const auto unset = qScopeGuard([&] {
        QDesktopServices::unsetUrlHandler(htmlTestFile.scheme());
        QDesktopServices::unsetUrlHandler("test");
    });

    QQmlComponent component(&engine, testFileUrl("openUrlExternally_lib.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QCOMPARE(handler.called,1);
    QCOMPARE(handler.last, QUrl("test:url"));

    object->setProperty("testFile", true);

    QCOMPARE(handler.called,2);
    QCOMPARE(handler.last, htmlTestFile);
}

void tst_qqmlqt::md5()
{
    QQmlComponent component(&engine, testFileUrl("md5.qml"));

    QString warning1 = component.url().toString() + ":4: Error: Insufficient arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(object->property("test2").toString(), QLatin1String(QCryptographicHash::hash("Hello World", QCryptographicHash::Md5).toHex()));
}

void tst_qqmlqt::createComponent()
{
    {
        QQmlComponent component(&engine, testFileUrl("createComponent.qml"));

        QString warning1 = "createComponent.qml:9: Error: Unable to determine callable overload";
        QString warning2 = component.url().toString() + ":10: Error: Invalid compilation mode 10";
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(warning1));
        QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));

        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);

        QCOMPARE(object->property("absoluteUrl").toString(), QString("http://www.example.com/test.qml"));
        QCOMPARE(object->property("relativeUrl").toString(), testFileUrl("createComponentData.qml").toString());

        QTRY_VERIFY(object->property("asyncResult").toBool());
    }

    // simultaneous sync and async compilation
    {
        QQmlComponent component(&engine, testFileUrl("createComponent.2.qml"));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(object != nullptr);
        QTRY_VERIFY(object->property("success").toBool());
    }
}

void tst_qqmlqt::createComponent_pragmaLibrary()
{
    // Currently, just loading createComponent_lib.qml causes crash on some platforms
    QQmlComponent component(&engine, testFileUrl("createComponent_lib.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QCOMPARE(object->property("status").toInt(), int(QQmlComponent::Ready));
    QCOMPARE(object->property("readValue").toInt(), int(1913));
}

void tst_qqmlqt::createQmlObject()
{
    QQmlComponent component(&engine, testFileUrl("createQmlObject.qml"));

    QString warning1 = "createQmlObject.qml:7: Error: Unable to determine callable overload";
    QString warning2 = component.url().toString()+ ":10: Error: Qt.createQmlObject(): failed to create object: \n    " + testFileUrl("inline").toString() + ":2:10: Blah is not a type";
    QString warning3 = component.url().toString()+ ":11: Error: Qt.createQmlObject(): failed to create object: \n    " + testFileUrl("main.qml").toString() + ":4:14: Duplicate property name";
    QString warning4 = component.url().toString()+ ":9: TypeError: Passing incompatible arguments to C++ functions from JavaScript is not allowed.";
    QString warning5 = component.url().toString()+ ":8: Error: Too many arguments";
    QString warning6 = "RunTimeError:  Qt.createQmlObject(): failed to create object: \n    " + testFileUrl("inline").toString() + ":3:16: Cannot assign object type QObject with no default method";
    QString warning7 = "Could not convert argument 1 at";
    QString warning8 = "expression for noParent@";
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(warning1));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning3));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning4));
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning5));
    QTest::ignoreMessage(QtDebugMsg, qPrintable(warning6));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(warning7));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(warning8));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(object->property("emptyArg").toBool(), true);
    QCOMPARE(object->property("success").toBool(), true);

    QQuickItem *item = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(item != nullptr);
    QCOMPARE(item->childItems().size(), 1);
}


void tst_qqmlqt::dateTimeConversion()
{
    QDate date(2008,12,24);
    QTime time(14,15,38,200);
    QDateTime dateTime(date, time);
    QDateTime dateTime0(QDate(2021, 7, 1).startOfDay());
    QDateTime dateTime0utc(QDate(2021, 7, 1).startOfDay(QTimeZone::UTC));
    QDateTime dateTime1(QDate(2021, 7, 31).startOfDay());
    QDateTime dateTime1utc(QDate(2021, 7, 31).startOfDay(QTimeZone::UTC));
    QDateTime dateTime2(QDate(2852,12,31), QTime(23,59,59,500));
    QDateTime dateTime3(QDate(2000,1,1), QTime(0,0,0,0));
    QDateTime dateTime4(QDate(2001,2,2), QTime(0,0,0,0));
    QDateTime dateTime5(QDate(1999,1,1), QTime(2,3,4,0));
    QDateTime dateTime6(QDate(2008,2,24), QTime(14,15,38,200));
    QDateTime dateTime7(QDate(1970, 1, 1), QTime(0, 0), QTimeZone::UTC);
    QDateTime dateTime8(QDate(1586, 2, 2), QTime(0, 0), QTimeZone::UTC);
    QDateTime dateTime9(QDate(955, 1, 1), QTime(0, 0), QTimeZone::UTC);
    QDateTime dateTime10(QDate(113, 2, 24), QTime(14, 15, 38, 200), QTimeZone::UTC);

    QQmlEngine eng;
    QQmlComponent component(&eng, testFileUrl("dateTimeConversion.qml"));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY2(obj != nullptr, qPrintable(component.errorString()));

    QCOMPARE(obj->property("qdate").toDate(), date);
    QCOMPARE(obj->property("qtime").toTime(), time);
    QCOMPARE(obj->property("qdatetime").toDateTime(), dateTime);
    QCOMPARE(obj->property("qdatetime0").toDateTime(), dateTime0);
    QCOMPARE(obj->property("qdatetime0utc").toDateTime(), dateTime0utc);
    QCOMPARE(obj->property("qdatetime1").toDateTime(), dateTime1);
    QCOMPARE(obj->property("qdatetime1utc").toDateTime(), dateTime1utc);
    QCOMPARE(obj->property("qdatetime2").toDateTime(), dateTime2);
    QCOMPARE(obj->property("qdatetime3").toDateTime(), dateTime3);
    QCOMPARE(obj->property("qdatetime4").toDateTime(), dateTime4);
    QCOMPARE(obj->property("qdatetime5").toDateTime(), dateTime5);
    QCOMPARE(obj->property("qdatetime6").toDateTime(), dateTime6);
    QCOMPARE(obj->property("qdatetime7").toDateTime(), dateTime7);
    QCOMPARE(obj->property("qdatetime8").toDateTime(), dateTime8);
    QCOMPARE(obj->property("qdatetime9").toDateTime(), dateTime9);
    QCOMPARE(obj->property("qdatetime10").toDateTime(), dateTime10);
}

void tst_qqmlqt::dateTimeFormatting()
{
    QFETCH(QString, method);
    QFETCH(QStringList, inputProperties);
    QFETCH(QStringList, expectedResults);

    QDate date(2008,12,24);
    QTime time(14,15,38,200);
    QDateTime dateTime(date, time);

    QQmlEngine eng;

    QQmlComponent component(&eng, testFileUrl("formatting.qml"));

    QStringList warnings;
    warnings
        << component.url().toString() + ":37: TypeError: Passing incompatible arguments to C++ functions from JavaScript is not allowed."
        << component.url().toString() + ":40: TypeError: Passing incompatible arguments to C++ functions from JavaScript is not allowed."
        << component.url().toString() + ":43: TypeError: Passing incompatible arguments to C++ functions from JavaScript is not allowed.";

    foreach (const QString &warning, warnings)
        QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));

    warnings.clear();
    warnings
        << "formatting.qml:36: Error: Unable to determine callable overload"
        << "formatting.qml:39: Error: Unable to determine callable overload"
        << "formatting.qml:42: Error: Unable to determine callable overload"
        << "Could not convert argument 1 at"
        << "expression for err_date2@"
        << "Could not convert argument 1 at"
        << "expression for err_time2@"
        << "Could not convert argument 1 at"
        << "expression for err_dateTime2@";

    foreach (const QString &warning, warnings)
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(warning));

    QScopedPointer<QObject> object(component.createWithInitialProperties({
            {"qdate", date},
            {"qtime", time},
            {"qdatetime", dateTime}
    }));
    QVERIFY2(component.errorString().isEmpty(), qPrintable(component.errorString()));
    QVERIFY(object != nullptr);

    QVERIFY(inputProperties.size() > 0);
    QVariant result;
    foreach(const QString &prop, inputProperties) {
        QVERIFY(QMetaObject::invokeMethod(object.data(), method.toUtf8().constData(),
                Q_RETURN_ARG(QVariant, result),
                Q_ARG(QVariant, prop)));
        QStringList output = result.toStringList();
        QCOMPARE(output.size(), expectedResults.size());
        for (int i=0; i<output.size(); i++)
            QCOMPARE(output[i], expectedResults[i]);
    }
}

void tst_qqmlqt::dateTimeFormatting_data()
{
    QTest::addColumn<QString>("method");
    QTest::addColumn<QStringList>("inputProperties");
    QTest::addColumn<QStringList>("expectedResults");

    QDate date(2008,12,24);
    QTime time(14,15,38,200);
    QDateTime dateTime(date, time);

    QTest::newRow("formatDate")
        << "formatDate"
        << (QStringList() << "dateFromString" << "jsdate" << "qdate" << "qdatetime")
        << (QStringList() << QLocale().toString(date, QLocale::ShortFormat)
                          << QLocale().toString(date, QLocale::LongFormat)
                          << date.toString("ddd MMMM d yy"));

    QTest::newRow("formatTime")
        << "formatTime"
        << (QStringList() << "jsdate" << "qtime" << "qdatetime")
        << (QStringList() << QLocale().toString(time, QLocale::ShortFormat)
                          << QLocale().toString(time, QLocale::LongFormat)
                          << time.toString("H:m:s a")
                          << time.toString("hh:mm:ss.zzz"));

    QTest::newRow("formatDateTime")
        << "formatDateTime"
        << (QStringList() << "jsdate" << "qdatetime")
        << (QStringList() << QLocale().toString(dateTime, QLocale::ShortFormat)
                          << QLocale().toString(dateTime, QLocale::LongFormat)
                          << dateTime.toString("M/d/yy H:m:s a"));
}

void tst_qqmlqt::dateTimeFormattingVariants()
{
    QFETCH(QString, method);
    QFETCH(QVariant, variant);
    QFETCH(QStringList, expectedResults);

    QQmlEngine eng;
    QQmlComponent component(&eng, testFileUrl("formatting.qml"));

    QStringList warnings;
    warnings << component.url().toString() + ":37: TypeError: Passing incompatible arguments to C++ functions from JavaScript is not allowed."
             << component.url().toString() + ":40: TypeError: Passing incompatible arguments to C++ functions from JavaScript is not allowed."
             << component.url().toString() + ":43: TypeError: Passing incompatible arguments to C++ functions from JavaScript is not allowed.";

    for (const QString &warning : std::as_const(warnings))
        QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));

    warnings.clear();
    warnings << "formatting.qml:36: Error: Unable to determine callable overload."
             << "formatting.qml:39: Error: Unable to determine callable overload."
             << "formatting.qml:42: Error: Unable to determine callable overload."
             << "Could not convert argument 1 at"
             << "expression for err_date2@"
             << "Could not convert argument 1 at"
             << "expression for err_time2@"
             << "Could not convert argument 1 at"
             << "expression for err_dateTime2@";

    for (const QString &warning : std::as_const(warnings))
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(warning));

    warnings.clear();

    if (method == QStringLiteral("formatTime")
            && variant.typeId() == QMetaType::QString
            && QByteArrayView(QTest::currentDataTag()).endsWith("ISO")) {
        for (int i = 0; i < 4; ++i) {
            QTest::ignoreMessage(QtWarningMsg,
                                 "\"2011/05/31 11:16:39.755\" is a "
                                 "date/time string being passed to formatTime(). You should only "
                                 "pass time strings to formatTime().");
        }
    }

    if (variant.typeId() == QMetaType::QColor || variant.typeId() == QMetaType::Int) {
        if (method == "formatTime") {
            // formatTime has special error handling as it parses the strings itself.
            QTest::ignoreMessage(
                    QtWarningMsg,
                    QRegularExpression("formatting.qml:19: Error: Invalid argument passed to "
                                       "formatTime"));
        } else if (method == "formatDate") {
            // formatDate has special error handling as it parses the strings itself.
            QTest::ignoreMessage(
                    QtWarningMsg,
                    QRegularExpression("formatting.qml:10: Error: Invalid argument passed to "
                                       "formatDate"));
        } else if (method == "formatDateTime") {
            // formatDateTime has special error handling as it parses the strings itself.
            QTest::ignoreMessage(
                    QtWarningMsg,
                    QRegularExpression("formatting.qml:29: Error: Invalid argument passed to "
                                       "formatDateTime"));
        }
    }

    QScopedPointer<QObject> object(component.createWithInitialProperties({{"qvariant", variant}}));
    QVERIFY2(component.errorString().isEmpty(), qPrintable(component.errorString()));
    QVERIFY(object != nullptr);

    QVariant result;
    QVERIFY(QMetaObject::invokeMethod(object.data(), method.toUtf8().constData(),
            Q_RETURN_ARG(QVariant, result),
            Q_ARG(QVariant, QString(QLatin1String("qvariant")))));
    QStringList output = result.toStringList();
    QCOMPARE(output, expectedResults);
}

void tst_qqmlqt::dateTimeFormattingVariants_data()
{
    QTest::addColumn<QString>("method");
    QTest::addColumn<QVariant>("variant");
    QTest::addColumn<QStringList>("expectedResults");

    QDateTime temporary;

    QTime time(11, 16, 39, 755);
    temporary = QDateTime(QDate(1970,1,1), time);
    QTest::newRow("formatTime, qtime")
        << "formatTime" << QVariant::fromValue(time)
        << (QStringList()
            << QLocale().toString(temporary.time(), QLocale::ShortFormat)
            << QLocale().toString(temporary.time(), QLocale::LongFormat)
            << temporary.time().toString("H:m:s a")
            << temporary.time().toString("hh:mm:ss.zzz"));

    QDate date(2011,5,31);
    // V4 reads the date in UTC but DateObject::toQDateTime() gives it back in local time:
    temporary = date.startOfDay(QTimeZone::UTC).toLocalTime();
    QTest::newRow("formatDate, qdate")
        << "formatDate" << QVariant::fromValue(date)
        << (QStringList()
            << QLocale().toString(temporary.date(), QLocale::ShortFormat)
            << QLocale().toString(temporary.date(), QLocale::LongFormat)
            << temporary.date().toString("ddd MMMM d yy"));
    QTest::newRow("formatDateTime, qdate")
        << "formatDateTime" << QVariant::fromValue(date)
        << (QStringList()
            << QLocale().toString(temporary, QLocale::ShortFormat)
            << QLocale().toString(temporary, QLocale::LongFormat)
            << temporary.toString("M/d/yy H:m:s a"));
    QTest::newRow("formatTime, qdate")
        << "formatTime" << QVariant::fromValue(date)
        << (QStringList()
            << QLocale().toString(temporary.time(), QLocale::ShortFormat)
            << QLocale().toString(temporary.time(), QLocale::LongFormat)
            << temporary
            .time().toString("H:m:s a") << temporary.time().toString("hh:mm:ss.zzz"));

    QDateTime dateTime(date, time);
    temporary = dateTime;
    QTest::newRow("formatDate, qdatetime")
        << "formatDate" << QVariant::fromValue(dateTime)
        << (QStringList()
            << QLocale().toString(temporary.date(), QLocale::ShortFormat)
            << QLocale().toString(temporary.date(), QLocale::LongFormat)
            << temporary.date().toString("ddd MMMM d yy"));
    QTest::newRow("formatDateTime, qdatetime")
        << "formatDateTime" << QVariant::fromValue(dateTime)
        << (QStringList()
            << QLocale().toString(temporary, QLocale::ShortFormat)
            << QLocale().toString(temporary, QLocale::LongFormat)
            << temporary.toString("M/d/yy H:m:s a"));
    QTest::newRow("formatTime, qdatetime")
        << "formatTime" << QVariant::fromValue(dateTime)
        << (QStringList()
            << QLocale().toString(temporary.time(), QLocale::ShortFormat)
            << QLocale().toString(temporary.time(), QLocale::LongFormat)
            << temporary.time().toString("H:m:s a")
            << temporary.time().toString("hh:mm:ss.zzz"));

    const QString isoString(QLatin1String("2011/05/31 11:16:39.755"));
    temporary = QDateTime::fromString(isoString, "yyyy/MM/dd HH:mm:ss.zzz");
    const QString jsString = engine.coerceValue<QDateTime, QString>(temporary);
    QTest::newRow("formatDate, qstring, ISO")
        << "formatDate" << QVariant::fromValue(isoString)
        << (QStringList()
            << QLocale().toString(temporary.date(), QLocale::ShortFormat)
            << QLocale().toString(temporary.date(), QLocale::LongFormat)
            << temporary.date().toString("ddd MMMM d yy"));
    QTest::newRow("formatDate, qstring, JS")
        << "formatDate" << QVariant::fromValue(jsString)
        << (QStringList()
            << QLocale().toString(temporary.date(), QLocale::ShortFormat)
            << QLocale().toString(temporary.date(), QLocale::LongFormat)
            << temporary.date().toString("ddd MMMM d yy"));
    QTest::newRow("formatDateTime, qstring, ISO")
        << "formatDateTime" << QVariant::fromValue(isoString)
        << (QStringList()
            << QLocale().toString(temporary, QLocale::ShortFormat)
            << QLocale().toString(temporary, QLocale::LongFormat)
            << temporary.toString("M/d/yy H:m:s a"));
    QTest::newRow("formatDateTime, qstring, JS")
        << "formatDateTime" << QVariant::fromValue(jsString)
        << (QStringList()
            << QLocale().toString(temporary, QLocale::ShortFormat)
            << QLocale().toString(temporary, QLocale::LongFormat)
            << temporary.toString("M/d/yy H:m:s a"));
    QTest::newRow("formatTime, qstring, ISO")
        << "formatTime" << QVariant::fromValue(isoString)
        << (QStringList()
            << QLocale().toString(temporary.time(), QLocale::ShortFormat)
            << QLocale().toString(temporary.time(), QLocale::LongFormat)
            << temporary.time().toString("H:m:s a")
            << temporary.time().toString("hh:mm:ss.zzz"));
    QTest::newRow("formatTime, qstring, JS")
        << "formatTime" << QVariant::fromValue(jsString)
        << (QStringList()
            << QLocale().toString(temporary.time(), QLocale::ShortFormat)
            << QLocale().toString(temporary.time(), QLocale::LongFormat)
            << temporary.time().toString("H:m:s a")
            << temporary.time().toString("hh:mm:ss.000")); // JS Date to string coercion drops milliseconds

    QColor color(Qt::red);
    temporary = QVariant::fromValue(color).toDateTime();
    QTest::newRow("formatDate, qcolor")
        << "formatDate" << QVariant::fromValue(color)
        << QStringList();
    QTest::newRow("formatDateTime, qcolor")
        << "formatDateTime" << QVariant::fromValue(color)
        << QStringList();
    QTest::newRow("formatTime, qcolor")
        << "formatTime" << QVariant::fromValue(color)
        << QStringList();

    int integer(4);
    temporary = QVariant::fromValue(integer).toDateTime();
    QTest::newRow("formatDate, int")
        << "formatDate" << QVariant::fromValue(integer)
        << QStringList();
    QTest::newRow("formatDateTime, int")
        << "formatDateTime" << QVariant::fromValue(integer)
        << QStringList();
    QTest::newRow("formatTime, int")
        << "formatTime" << QVariant::fromValue(integer)
        << QStringList();
}

void tst_qqmlqt::dateTimeFormattingWithLocale()
{
    QQmlEngine engine;
    auto url = testFileUrl("formattingLocale.qml");
    QQmlComponent comp(&engine, url);
    QDateTime dateTime = QDateTime::fromString("M1d1y9800:01:02",
                                               "'M'M'd'd'y'yyhh:mm:ss");
    QDate date(1995, 5, 17);
    QScopedPointer<QObject> o(comp.createWithInitialProperties({ {"myDateTime", dateTime}, {"myDate", date} }));
    QVERIFY(!o.isNull());

    auto dateTimeString = o->property("dateTimeString").toString();
    QCOMPARE(dateTimeString, QLocale("de_DE").toString(dateTime, QLocale::NarrowFormat));
    auto dateString = o->property("dateString").toString();
    QCOMPARE(dateString, QLocale("de_DE").toString(date, QLocale::ShortFormat));

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Could not convert argument 1 at"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("invalidUsage@"));
    QTest::ignoreMessage(
                QtWarningMsg,
                qPrintable(url.toString() + QStringLiteral(":11: TypeError: Passing incompatible "
                                                           "arguments to C++ functions from "
                                                           "JavaScript is not allowed.")));
    QMetaObject::invokeMethod(o.get(), "invalidUsage");
}

void tst_qqmlqt::isQtObject()
{
    QQmlComponent component(&engine, testFileUrl("isQtObject.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(object->property("test1").toBool(), true);
    QCOMPARE(object->property("test2").toBool(), false);
    QCOMPARE(object->property("test3").toBool(), false);
    QCOMPARE(object->property("test4").toBool(), false);
    QCOMPARE(object->property("test5").toBool(), false);
}

void tst_qqmlqt::btoa()
{
    QQmlComponent component(&engine, testFileUrl("btoa.qml"));

    QString warning1 = component.url().toString() + ":4: Error: Insufficient arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(object->property("test2").toString(), QString("SGVsbG8gd29ybGQh"));
}

void tst_qqmlqt::atob()
{
    QQmlComponent component(&engine, testFileUrl("atob.qml"));

    QString warning1 = component.url().toString() + ":4: Error: Insufficient arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(object->property("test2").toString(), QString("Hello world!"));
}

void tst_qqmlqt::fontFamilies()
{
    QQmlComponent component(&engine, testFileUrl("fontFamilies.qml"));

    QString warning1 = component.url().toString() + ":4: Error: Too many arguments";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(object->property("test2"), QVariant::fromValue(QFontDatabase::families()));
}

void tst_qqmlqt::quit()
{
    QQmlComponent component(&engine, testFileUrl("quit.qml"));

    QSignalSpy spy(&engine, SIGNAL(quit()));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QCOMPARE(spy.size(), 1);
}

void tst_qqmlqt::exit()
{
    QQmlComponent component(&engine, testFileUrl("exit.qml"));

    QSignalSpy spy(&engine, &QQmlEngine::exit);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);
    QCOMPARE(spy.size(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toInt() == object->property("returnCode").toInt());
}

void tst_qqmlqt::resolvedUrl()
{
    QQmlEngine engine;
    engine.addImportPath(dataDirectory());
    QQmlComponent component(&engine, testFileUrl("resolvedUrl.qml"));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(object != nullptr);

    QCOMPARE(object->property("result").toString(), component.url().toString());
    QCOMPARE(object->property("isString").toBool(), false);
    QCOMPARE(object->property("isObject").toBool(), true);

    QCOMPARE(qvariant_cast<QUrl>(object->property("resolvedHere")),
             dataDirectoryUrl().resolved(QStringLiteral("somewhere.qml")));
    QCOMPARE(qvariant_cast<QUrl>(object->property("resolvedThere")),
             dataDirectoryUrl().resolved(QStringLiteral("Other/somewhere.qml")));

    QVariant unresolved = object->property("unresolvedUrl");
    QCOMPARE(unresolved.metaType(), QMetaType::fromType<QUrl>());
    QCOMPARE(qvariant_cast<QUrl>(unresolved), QUrl(QStringLiteral("nowhere/else.js")));
}

void tst_qqmlqt::later_data()
{
    QTest::addColumn<QString>("function");
    QTest::addColumn<QStringList>("expectedWarnings");
    QTest::addColumn<QStringList>("propNames");
    QTest::addColumn<QVariantList>("values");

    QVariant vtrue = QVariant(true);

    QTest::newRow("callLater from onCompleted")
            << QString()
            << QStringList()
            << (QStringList() << "test1_1" << "test2_1" << "processEvents" << "test1_2" << "test2_2")
            << (QVariantList() << vtrue << vtrue << QVariant() << vtrue << vtrue);

    QTest::newRow("trigger Qt.callLater() via repeater")
            << QString(QLatin1String("test2"))
            << QStringList()
            << (QStringList() << "processEvents" << "test2_2")
            << (QVariantList() << QVariant() << vtrue);

    QTest::newRow("recursive Qt.callLater()")
            << QString(QLatin1String("test3"))
            << QStringList()
            << (QStringList() << "processEvents" << "test3_1" << "processEvents" << "test3_2" << "processEvents" << "test3_3")
            << (QVariantList() << QVariant() << vtrue << QVariant() << vtrue << QVariant() << vtrue);

    QTest::newRow("nonexistent function")
            << QString(QLatin1String("test4"))
            << (QStringList() << QString(testFileUrl("later.qml").toString() + QLatin1String(":70: ReferenceError: functionThatDoesNotExist is not defined")))
            << QStringList()
            << QVariantList();

    QTest::newRow("callLater with different args")
            << QString(QLatin1String("test5"))
            << QStringList()
            << (QStringList() << "processEvents" << "test5_1")
            << (QVariantList() << QVariant() << vtrue);

    QTest::newRow("delayed call ordering")
            << QString(QLatin1String("test6"))
            << QStringList()
            << (QStringList() << "processEvents" << "test6_1")
            << (QVariantList() << QVariant() << vtrue);

    QTest::newRow("invoke module api invokable")
            << QString(QLatin1String("test9"))
            << QStringList()
            << (QStringList() << "processEvents" << "test9_1" << "processEvents")
            << (QVariantList() << QVariant() << QVariant(1) << QVariant());

    QTest::newRow("invoke function of deleted QObject via callLater() causing deletion")
            << QString(QLatin1String("test10"))
            << (QStringList() << QString(testFileUrl("LaterComponent.qml").toString() + QLatin1String(":8: ReferenceError: dangerousFunction is not defined (exception occurred during delayed function evaluation)")))
            << (QStringList() << "processEvents" << "test10_1" << "processEvents")
            << (QVariantList() << QVariant() << QVariant(0) << QVariant());

    QTest::newRow("invoke function of deleted QObject via callLater() after deletion")
            << QString(QLatin1String("test11"))
            << QStringList()
            << (QStringList() << "collectGarbage" << "processEvents" << "test11_1" << "processEvents")
            << (QVariantList() << QVariant() << QVariant() << QVariant(1) << QVariant());

    QTest::newRow("invoke function which has no script origin")
            << QString(QLatin1String("test14"))
            << QStringList()
            << (QStringList() << "collectGarbage")
            << (QVariantList() << QVariant());
}

void tst_qqmlqt::later()
{
    QFETCH(QString, function);
    QFETCH(QStringList, expectedWarnings);
    QFETCH(QStringList, propNames);
    QFETCH(QVariantList, values);

    foreach (const QString &w, expectedWarnings)
        QTest::ignoreMessage(QtWarningMsg, qPrintable(w));

    QQmlComponent component(&engine, testFileUrl("later.qml"));
    QScopedPointer<QObject> root(component.create());
    QVERIFY(root != nullptr);

    if (!function.isEmpty())
        QMetaObject::invokeMethod(root.data(), qPrintable(function));

    for (int i = 0; i < propNames.size(); ++i) {
        if (propNames.at(i) == QLatin1String("processEvents")) {
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            QCoreApplication::processEvents();
        } else if (propNames.at(i) == QLatin1String("collectGarbage")) {
            engine.collectGarbage();
        } else {
            QCOMPARE(root->property(qPrintable(propNames.at(i))), values.at(i));
        }
    }
}

void tst_qqmlqt::qtObjectContents()
{
    QByteArray qml =
            "import QtQml\n"
            "QtObject {\n"
            "    property int vLoadingModeAsynchronous: Qt.Asynchronous\n"
            "    property int vLoadingModeSynchronous: Qt.Synchronous\n";

    const QMetaObject *qtMetaObject = &Qt::staticMetaObject;
    for (int ii = 0; ii < qtMetaObject->enumeratorCount(); ++ii) {
        const QMetaEnum enumerator = qtMetaObject->enumerator(ii);
        for (int jj = 0; jj < enumerator.keyCount(); ++jj) {
            const QByteArray key = enumerator.key(jj);
            QVERIFY(!key.isEmpty());

            // We don't want to check for Qt.green and things like that.
            // They're nonsensical
            if (QChar::fromLatin1(key.front()).isLower())
                continue;

            qml += QByteArray("    property int v") + enumerator.name() + key
                    + QByteArray(": Qt.") + key + '\n';
        }
    }

    qml += "}\n";

    QQmlComponent component(&engine);
    component.setData(qml, QUrl());

    QScopedPointer<QObject> object(component.create());
    QVERIFY2(object != nullptr, qPrintable(component.errorString()));

    bool ok = false;
    for (int ii = 0; ii < qtMetaObject->enumeratorCount(); ++ii) {
        const QMetaEnum enumerator = qtMetaObject->enumerator(ii);
        for (int jj = 0; jj < enumerator.keyCount(); ++jj) {
            const QByteArray key = enumerator.key(jj);

            if (QChar::fromLatin1(key.front()).isLower())
                continue;

            QCOMPARE(object->property(QByteArray("v") + enumerator.name() + key).toInt(&ok),
                     enumerator.value(jj));
            QVERIFY(ok);
        }
    }

    QCOMPARE(object->property("vLoadingModeAsynchronous").toInt(&ok), 0);
    QVERIFY(ok);
    QCOMPARE(object->property("vLoadingModeSynchronous").toInt(&ok), 1);
    QVERIFY(ok);
}

class TimeProvider: public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(TimeProvider)
    QML_UNCREATABLE("")
    Q_PROPERTY(QTime time READ time WRITE setTime NOTIFY timeChanged)

public:
    TimeProvider(QTime t)
        : m_getTime(t)
    {}

    QTime time() const { return m_getTime; }
    void setTime(QTime t) { m_putTime = t; emit timeChanged(); }

signals:
    void timeChanged();

public:
    QTime m_getTime, m_putTime;
};

class TimeZoneSwitch
{
public:
    TimeZoneSwitch(const char *newZone)
        : doChangeZone(qstrcmp(newZone, "localtime") != 0)
    {
        if (!doChangeZone)
            return;

        hadOldZone = qEnvironmentVariableIsSet("TZ");
        if (hadOldZone) {
            oldZone = qgetenv("TZ");
        }
        qputenv("TZ", newZone);
        qTzSet();
    }

    ~TimeZoneSwitch()
    {
        if (!doChangeZone)
            return;

        if (hadOldZone)
            qputenv("TZ", oldZone);
        else
            qunsetenv("TZ");
        qTzSet();
    }

private:
    bool doChangeZone;
    bool hadOldZone;
    QByteArray oldZone;
};

void tst_qqmlqt::timeRoundtrip_data()
{
    QTest::addColumn<QTime>("time");

    // Local timezone:
    QTest::newRow("localtime") << QTime(0, 0, 0);

#if defined(Q_OS_WIN) || defined(Q_OS_ANDROID)
    qInfo("Omitting the tests that depend on setting local time's zone");
#else
    // No DST:
    QTest::newRow("UTC") << QTime(0, 0, 0);
    QTest::newRow("Europe/Amsterdam") << QTime(1, 0, 0);
    QTest::newRow("Asia/Jakarta") << QTime(7, 0, 0);

    // DST:
    QTest::newRow("Namibia/Windhoek") << QTime(1, 0, 0);
    QTest::newRow("Australia/Adelaide") << QTime(10, 0, 0);
    QTest::newRow("Australia/Hobart") << QTime(10, 0, 0);
    QTest::newRow("Pacific/Auckland") << QTime(12, 0, 0);
    QTest::newRow("Pacific/Samoa") << QTime(13, 0, 0);
#endif
}

void tst_qqmlqt::timeRoundtrip()
{
    TimeZoneSwitch tzs(QTest::currentDataTag());
    QFETCH(QTime, time);
    qmlRegisterTypesAndRevisions<TimeProvider>("Test", 1);

    TimeProvider tp(time);

    QQmlEngine eng;
    //qmlRegisterSingletonInstance("Test", 1, 0, "TimeProvider", &tp);
    QQmlComponent component(&eng, testFileUrl("timeRoundtrip.qml"));
    QScopedPointer<QObject> obj(component.createWithInitialProperties(
            {{"tp", QVariant::fromValue(&tp)}}));
    QVERIFY(obj != nullptr);

    // QML reads m_getTime and saves the result as m_putTime; this should come out the same, without
    // any perturbation (e.g. by DST effects) from converting from QTime to V4's Date and back
    // again.
    QCOMPARE(tp.m_getTime, tp.m_putTime);
}

QTEST_MAIN(tst_qqmlqt)

#include "tst_qqmlqt.moc"
