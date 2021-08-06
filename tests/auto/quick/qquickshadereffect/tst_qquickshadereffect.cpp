/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>

#include <QList>
#include <QByteArray>
#include <private/qquickshadereffect_p.h>
#include <QMatrix4x4>
#include <QtQuick/QQuickView>
#include <QtQml/QQmlEngine>
#include <QtQuickTestUtils/private/qmlutils_p.h>

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
    QQuickView *view = new QQuickView(nullptr);
    view->setSource(QUrl(QStringLiteral("qrc:/data/connections.qml")));

    auto *shaderEffectItem = qobject_cast<TestShaderEffect*>(view->rootObject());
    QVERIFY(shaderEffectItem);
    QCOMPARE(shaderEffectItem->signalsConnected, 0);

    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view));

    QSGRendererInterface *rif = view->rendererInterface();
    if (rif && rif->graphicsApi() != QSGRendererInterface::Software)
        QCOMPARE(shaderEffectItem->signalsConnected, 1);
}

void tst_qquickshadereffect::deleteSourceItem()
{
    // purely to ensure that deleting the sourceItem of a shader doesn't cause a crash
    QQuickView *view = new QQuickView(nullptr);
    view->setSource(QUrl(QStringLiteral("qrc:/data/deleteSourceItem.qml")));
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view));
    QVERIFY(view);
    QObject *obj = view->rootObject();
    QVERIFY(obj);
    QMetaObject::invokeMethod(obj, "setDeletedSourceItem");
    QTest::qWait(50);
    delete view;
}

void tst_qquickshadereffect::deleteShaderEffectSource()
{
    // purely to ensure that deleting the sourceItem of a shader doesn't cause a crash
    QQuickView *view = new QQuickView(nullptr);
    view->setSource(QUrl(QStringLiteral("qrc:/data/deleteShaderEffectSource.qml")));
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view));
    QVERIFY(view);
    QObject *obj = view->rootObject();
    QVERIFY(obj);
    QMetaObject::invokeMethod(obj, "setDeletedShaderEffectSource");
    QTest::qWait(50);
    delete view;
}

void tst_qquickshadereffect::twoImagesOneShaderEffect()
{
    // Don't crash when having a ShaderEffect and an Image sharing the texture via supportsAtlasTextures
    QQuickView *view = new QQuickView(nullptr);
    view->setSource(QUrl(QStringLiteral("qrc:/data/twoImagesOneShaderEffect.qml")));
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view));
    QVERIFY(view);
    QObject *obj = view->rootObject();
    QVERIFY(obj);
    delete view;
}

void tst_qquickshadereffect::withoutQmlEngine()
{
    // using a shader without QML engine used to crash
    auto window = new QQuickWindow;
    auto shaderEffect = new TestShaderEffect(window->contentItem());
    shaderEffect->setVertexShader(QUrl());
    QVERIFY(shaderEffect->isComponentComplete());
    delete window;
}

// QTBUG-86402: hiding the parent of an item that uses an effect should not cause a crash.
void tst_qquickshadereffect::hideParent()
{
    QScopedPointer<QQuickView> view(new QQuickView);
    view->setSource(QUrl(QStringLiteral("qrc:/data/hideParent.qml")));
    QCOMPARE(view->status(), QQuickView::Ready);
    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.data()));
    // Should finish without crashing.
    QTRY_VERIFY(view->rootObject()->property("finished").toBool());
}

QTEST_MAIN(tst_qquickshadereffect)

#include "tst_qquickshadereffect.moc"
