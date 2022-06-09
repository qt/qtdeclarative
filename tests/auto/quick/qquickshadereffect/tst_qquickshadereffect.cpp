// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>

#include <QList>
#include <QByteArray>
#include <private/qquickshadereffect_p.h>
#include <QMatrix4x4>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickView>
#include <QtQuick/private/qquickshadereffectsource_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>

class TestShaderEffect : public QQuickShaderEffect
{
    Q_OBJECT
    Q_PROPERTY(QVariant source READ dummyRead NOTIFY sourceChanged)
    Q_PROPERTY(QVariant _0aA9zZ READ dummyRead NOTIFY dummyChanged)
    Q_PROPERTY(QVariant x86 READ dummyRead NOTIFY dummyChanged)
    Q_PROPERTY(QVariant X READ dummyRead NOTIFY dummyChanged)
    Q_PROPERTY(QMatrix4x4 mat4x4 READ mat4x4Read NOTIFY dummyChanged)

public:
    TestShaderEffect(QQuickItem* parent = nullptr) : QQuickShaderEffect(parent)
    {
    }

    QMatrix4x4 mat4x4Read() const { return QMatrix4x4(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1); }
    QVariant dummyRead() const { return QVariant(); }

    int signalsConnected = 0;

protected:
    void connectNotify(const QMetaMethod &s) override {
        if (s.name() == "sourceChanged")
            ++signalsConnected;
    }
    void disconnectNotify(const QMetaMethod &s) override
    {
        if (s.name() == "sourceChanged")
            --signalsConnected;
    }

signals:
    void dummyChanged();
    void sourceChanged();

private:
};

class tst_qquickshadereffect : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickshadereffect();

private slots:
    void initTestCase() override;
    void cleanupTestCase();
    void testConnection();
    void deleteSourceItem();
    void deleteShaderEffectSource();
    void twoImagesOneShaderEffect();

    void withoutQmlEngine();

    void hideParent();
    void testPropertyMappings();

private:
    enum PresenceFlags {
        VertexPresent = 0x01,
        TexCoordPresent = 0x02,
        MatrixPresent = 0x04,
        OpacityPresent = 0x08,
        SourcePresent = 0x10
    };
};

tst_qquickshadereffect::tst_qquickshadereffect()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
    qmlRegisterType<TestShaderEffect>("ShaderEffectTest", 1, 0, "TestShaderEffect");
}

void tst_qquickshadereffect::initTestCase()
{
    QQmlDataTest::initTestCase();
}

void tst_qquickshadereffect::cleanupTestCase()
{
}

void tst_qquickshadereffect::testConnection()
{
    // verify that the property notify signal is connected
    QQuickView view;
    QVERIFY(QQuickTest::initView(view, QStringLiteral("qrc:/data/connections.qml")));

    auto *shaderEffectItem = qobject_cast<TestShaderEffect*>(view.rootObject());
    QVERIFY(shaderEffectItem);
    QCOMPARE(shaderEffectItem->signalsConnected, 0);

    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QSGRendererInterface *rif = view.rendererInterface();
    if (rif && rif->graphicsApi() != QSGRendererInterface::Software)
        QCOMPARE(shaderEffectItem->signalsConnected, 1);
}

void tst_qquickshadereffect::deleteSourceItem()
{
    // purely to ensure that deleting the sourceItem of a shader doesn't cause a crash
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, QStringLiteral("qrc:/data/deleteSourceItem.qml")));
    QObject *obj = view.rootObject();
    QVERIFY(obj);
    QMetaObject::invokeMethod(obj, "setDeletedSourceItem");
    const auto shaderEffectSource = obj->findChild<QQuickShaderEffectSource*>();
    QVERIFY(shaderEffectSource);
    QTRY_VERIFY(!shaderEffectSource->sourceItem());
}

void tst_qquickshadereffect::deleteShaderEffectSource()
{
    // purely to ensure that deleting the ShaderEffectSource doesn't cause a crash
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, QStringLiteral("qrc:/data/deleteShaderEffectSource.qml")));
    QObject *obj = view.rootObject();
    QVERIFY(obj);
    const QPointer<QQuickShaderEffectSource> shaderEffectSource = obj->findChild<QQuickShaderEffectSource*>();
    QVERIFY(shaderEffectSource);
    QMetaObject::invokeMethod(obj, "setDeletedShaderEffectSource");
    QTRY_VERIFY(shaderEffectSource);
}

void tst_qquickshadereffect::twoImagesOneShaderEffect()
{
    // Don't crash when having a ShaderEffect and an Image sharing the texture via supportsAtlasTextures
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, QStringLiteral("qrc:/data/twoImagesOneShaderEffect.qml")));
    QObject *obj = view.rootObject();
    QVERIFY(obj);
}

void tst_qquickshadereffect::withoutQmlEngine()
{
    // using a shader without QML engine used to crash
    const QQuickWindow window;
    auto shaderEffect = new TestShaderEffect(window.contentItem());
    shaderEffect->setVertexShader(QUrl());
    QVERIFY(shaderEffect->isComponentComplete());
}

// QTBUG-86402: hiding the parent of an item that uses an effect should not cause a crash.
void tst_qquickshadereffect::hideParent()
{
    QQuickView view;
    view.setSource(QUrl(QStringLiteral("qrc:/data/hideParent.qml")));
    QVERIFY(QQuickTest::showView(view, QStringLiteral("qrc:/data/hideParent.qml")));
    // Should finish without crashing.
    QTRY_VERIFY(view.rootObject()->property("finished").toBool());
}

void tst_qquickshadereffect::testPropertyMappings()
{
    QQuickView view;
    view.setSource(QUrl(QStringLiteral("qrc:/data/testProperties.qml")));
    QTRY_VERIFY(view.rootObject()->property("finished").toBool());
}

QTEST_MAIN(tst_qquickshadereffect)

#include "tst_qquickshadereffect.moc"
