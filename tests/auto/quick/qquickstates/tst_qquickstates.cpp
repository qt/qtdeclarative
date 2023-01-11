// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <qtest.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickview.h>
#include <private/qquickstateoperations_p.h>
#include <private/qquickanchors_p_p.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include <private/qquickimage_p.h>
#include <QtQuick/private/qquickpropertychanges_p.h>
#include <QtQuick/private/qquickstategroup_p.h>
#include <private/qquickitem_p.h>
#include <private/qqmlproperty_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtTest/qsignalspy.h>

class MyAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int foo READ foo WRITE setFoo)
public:
    MyAttached(QObject *parent) : QObject(parent), m_foo(13) {}

    int foo() const { return m_foo; }
    void setFoo(int f) { m_foo = f; }

private:
    int m_foo;
};

class MyRect : public QQuickRectangle
{
   Q_OBJECT
   Q_PROPERTY(int propertyWithNotify READ propertyWithNotify WRITE setPropertyWithNotify NOTIFY oddlyNamedNotifySignal)
public:
    MyRect() {}

    void doSomething() { emit didSomething(); }

    int propertyWithNotify() const { return m_prop; }
    void setPropertyWithNotify(int i) { m_prop = i; emit oddlyNamedNotifySignal(); }

    static MyAttached *qmlAttachedProperties(QObject *o) {
        return new MyAttached(o);
    }
Q_SIGNALS:
    void didSomething();
    void oddlyNamedNotifySignal();

private:
    int m_prop;
};

class MyBindable : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(int prop READ prop WRITE setProp BINDABLE bindableProp)
public:
    int prop() {return m_prop; }
    void setProp(int i) { m_prop = i; }
    QBindable<int> bindableProp() { return &m_prop; }
    Q_OBJECT_BINDABLE_PROPERTY(MyBindable, int, m_prop);
};

QML_DECLARE_TYPE(MyRect)
QML_DECLARE_TYPEINFO(MyRect, QML_HAS_ATTACHED_PROPERTIES)

class RemovableObj : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int prop READ prop WRITE setProp NOTIFY propChanged)

public:
    RemovableObj(QObject *parent) : QObject(parent), m_prop(4321) { }
    int prop() const { return m_prop; }

public slots:
    void setProp(int prop)
    {
        if (m_prop == prop)
            return;

        m_prop = prop;
        emit propChanged(m_prop);
    }

signals:
    void propChanged(int prop);

private:
    int m_prop;
};

class ContainingObj : public QObject
{
    Q_OBJECT
    Q_PROPERTY(RemovableObj *obj READ obj NOTIFY objChanged)
    RemovableObj *m_obj;

public:
    ContainingObj() : m_obj(new RemovableObj(this)) { }
    RemovableObj *obj() const { return m_obj; }

    Q_INVOKABLE void reset()
    {
        if (m_obj) {
            m_obj->deleteLater();
        }

        m_obj = new RemovableObj(this);
        emit objChanged();
    }
signals:
    void objChanged();
};

class tst_qquickstates : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickstates() : QQmlDataTest(QT_QMLTEST_DATADIR)
    {
#ifdef QML_DISABLE_INTERNAL_DEFERRED_PROPERTIES
    qputenv("QML_DISABLE_INTERNAL_DEFERRED_PROPERTIES", "1");
#endif
    }

private:
    QByteArray fullDataPath(const QString &path) const;

private slots:
    void initTestCase() override;

    void basicChanges();
    void attachedPropertyChanges();
    void basicExtension();
    void basicBinding();
    void signalOverride();
    void signalOverrideCrash();
    void signalOverrideCrash2();
    void signalOverrideCrash3();
    void signalOverrideCrash4();
    void parentChange();
    void parentChangeErrors();
    void anchorChanges();
    void anchorChanges2();
    void anchorChanges3();
    void anchorChanges4();
    void anchorChanges5();
    void anchorChangesRTL();
    void anchorChangesRTL2();
    void anchorChangesRTL3();
    void anchorChangesCrash();
    void anchorRewindBug();
    void anchorRewindBug2();
    void script();
    void restoreEntryValues();
    void explicitChanges();
    void propertyErrors();
    void incorrectRestoreBug();
    void autoStateAtStartupRestoreBug();
    void deletingChange();
    void deletingState();
    void tempState();
    void illegalTempState();
    void nonExistantProperty();
    void reset();
    void illegalObjectCreation();
    void whenOrdering();
    void urlResolution();
    void unnamedWhen();
    void returnToBase();
    void extendsBug();
    void editProperties();
    void QTBUG_14830();
    void avoidFastForward();
    void revertListBug();
    void QTBUG_38492();
    void revertListMemoryLeak();
    void duplicateStateName();
    void trivialWhen();
    void jsValueWhen_data();
    void jsValueWhen();
    void noStateOsciallation();
    void parentChangeCorrectReversal();
    void revertNullObjectBinding();
    void bindableProperties();
    void parentChangeInvolvingBindings();
    void deferredProperties();
    void rewindAnchorChange();
    void rewindAnchorChangeSize();
    void bindingProperlyRemovedWithTransition();
};

void tst_qquickstates::initTestCase()
{
    QQmlDataTest::initTestCase();
    qmlRegisterType<MyRect>("Qt.test", 1, 0, "MyRectangle");
    qmlRegisterSingletonType<ContainingObj>(
            "Qt.test", 1, 0, "ContainingObj", [](QQmlEngine *engine, QJSEngine *) {
                static ContainingObj instance;
                engine->setObjectOwnership(&instance, QQmlEngine::CppOwnership);
                return &instance;
            });
    qmlRegisterUncreatableType<RemovableObj>("Qt.test", 1, 0, "RemovableObj", "Uncreatable");
}

QByteArray tst_qquickstates::fullDataPath(const QString &path) const
{
    return testFileUrl(path).toString().toUtf8();
}

void tst_qquickstates::basicChanges()
{
    QQmlEngine engine;

    {
        QQmlComponent rectComponent(&engine, testFileUrl("basicChanges.qml"));
        QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
        QVERIFY(rect != nullptr);
        QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());

        QCOMPARE(rect->color(),QColor("red"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("red"));
    }

    {
        QQmlComponent rectComponent(&engine, testFileUrl("basicChanges2.qml"));
        QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
        QVERIFY(rect != nullptr);
        QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());

        QCOMPARE(rect->color(),QColor("red"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));

        rectPrivate->setState("green");
        QCOMPARE(rect->color(),QColor("green"));

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("red"));

        rectPrivate->setState("green");
        QCOMPARE(rect->color(),QColor("green"));
    }

    {
        QQmlComponent rectComponent(&engine, testFileUrl("basicChanges3.qml"));
        QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
        QVERIFY(rect != nullptr);
        QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());

        QCOMPARE(rect->color(),QColor("red"));
        QCOMPARE(rect->border()->width(),1.0);

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));
        QCOMPARE(rect->border()->width(),1.0);

        rectPrivate->setState("bordered");
        QCOMPARE(rect->color(),QColor("red"));
        QCOMPARE(rect->border()->width(),2.0);

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("red"));
        QCOMPARE(rect->border()->width(),1.0);
        //### we should be checking that this is an implicit rather than explicit 1 (which currently fails)

        rectPrivate->setState("bordered");
        QCOMPARE(rect->color(),QColor("red"));
        QCOMPARE(rect->border()->width(),2.0);

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));
        QCOMPARE(rect->border()->width(),1.0);

    }

    {
        // Test basicChanges4.qml can magically connect to propertyWithNotify's notify
        // signal using 'onPropertyWithNotifyChanged' even though the signal name is
        // actually 'oddlyNamedNotifySignal'

        QQmlComponent component(&engine, testFileUrl("basicChanges4.qml"));
        QVERIFY(component.isReady());

        QScopedPointer<MyRect> rect(qobject_cast<MyRect*>(component.create()));
        QVERIFY(rect != nullptr);

        QMetaProperty prop = rect->metaObject()->property(rect->metaObject()->indexOfProperty("propertyWithNotify"));
        QVERIFY(prop.hasNotifySignal());
        QString notifySignal = prop.notifySignal().methodSignature();
        QVERIFY(!notifySignal.startsWith("propertyWithNotifyChanged("));

        QCOMPARE(rect->color(), QColor(Qt::red));

        rect->setPropertyWithNotify(100);
        QCOMPARE(rect->color(), QColor(Qt::blue));
    }
}

void tst_qquickstates::attachedPropertyChanges()
{
    QQmlEngine engine;

    QQmlComponent component(&engine, testFileUrl("attachedPropertyChanges.qml"));
    QVERIFY(component.isReady());

    QScopedPointer<QQuickItem> item(qobject_cast<QQuickItem*>(component.create()));
    QVERIFY(item != nullptr);
    QCOMPARE(item->width(), 50.0);

    // Ensure attached property has been changed
    QObject *attObj = qmlAttachedPropertiesObject<MyRect>(item.get(), false);
    QVERIFY(attObj);

    MyAttached *att = qobject_cast<MyAttached*>(attObj);
    QVERIFY(att);

    QCOMPARE(att->foo(), 1);
}

void tst_qquickstates::basicExtension()
{
    QQmlEngine engine;

    {
        QQmlComponent rectComponent(&engine, testFileUrl("basicExtension.qml"));
        QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
        QVERIFY(rect != nullptr);
        QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());

        QCOMPARE(rect->color(),QColor("red"));
        QCOMPARE(rect->border()->width(),1.0);

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));
        QCOMPARE(rect->border()->width(),1.0);

        rectPrivate->setState("bordered");
        QCOMPARE(rect->color(),QColor("blue"));
        QCOMPARE(rect->border()->width(),2.0);

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));
        QCOMPARE(rect->border()->width(),1.0);

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("red"));
        QCOMPARE(rect->border()->width(),1.0);

        rectPrivate->setState("bordered");
        QCOMPARE(rect->color(),QColor("blue"));
        QCOMPARE(rect->border()->width(),2.0);

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("red"));
        QCOMPARE(rect->border()->width(),1.0);
    }

    {
        QQmlComponent rectComponent(&engine, testFileUrl("fakeExtension.qml"));
        QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
        QVERIFY(rect != nullptr);
        QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());

        QCOMPARE(rect->color(),QColor("red"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));

        rectPrivate->setState("green");
        QCOMPARE(rect->color(),QColor("green"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));

        rectPrivate->setState("green");
        QCOMPARE(rect->color(),QColor("green"));

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("red"));

        rectPrivate->setState("green");
        QCOMPARE(rect->color(),QColor("green"));
    }
}

void tst_qquickstates::basicBinding()
{
    QQmlEngine engine;

    {
        QQmlComponent rectComponent(&engine, testFileUrl("basicBinding.qml"));
        QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
        QVERIFY(rect != nullptr);
        QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());

        QCOMPARE(rect->color(),QColor("red"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("red"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));
        rect->setProperty("sourceColor", QColor("green"));
        QCOMPARE(rect->color(),QColor("green"));

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("red"));
        rect->setProperty("sourceColor", QColor("yellow"));
        QCOMPARE(rect->color(),QColor("red"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("yellow"));
    }

    {
        QQmlComponent rectComponent(&engine, testFileUrl("basicBinding2.qml"));
        QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
        QVERIFY(rect != nullptr);
        QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());

        QCOMPARE(rect->color(),QColor("red"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("red"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));
        rect->setProperty("sourceColor", QColor("green"));
        QCOMPARE(rect->color(),QColor("blue"));

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("green"));
        rect->setProperty("sourceColor", QColor("yellow"));
        QCOMPARE(rect->color(),QColor("yellow"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("yellow"));
    }

    {
        QQmlComponent rectComponent(&engine, testFileUrl("basicBinding3.qml"));
        QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
        QVERIFY(rect != nullptr);
        QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());

        QCOMPARE(rect->color(),QColor("red"));
        rect->setProperty("sourceColor", QColor("green"));
        QCOMPARE(rect->color(),QColor("green"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));
        rect->setProperty("sourceColor", QColor("red"));
        QCOMPARE(rect->color(),QColor("blue"));
        rect->setProperty("sourceColor2", QColor("yellow"));
        QCOMPARE(rect->color(),QColor("yellow"));

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("red"));
        rect->setProperty("sourceColor2", QColor("green"));
        QCOMPARE(rect->color(),QColor("red"));
        rect->setProperty("sourceColor", QColor("yellow"));
        QCOMPARE(rect->color(),QColor("yellow"));
    }

    {
        QQmlComponent rectComponent(&engine, testFileUrl("basicBinding4.qml"));
        QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
        QVERIFY(rect != nullptr);
        QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());

        QCOMPARE(rect->color(),QColor("red"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));
        rect->setProperty("sourceColor", QColor("yellow"));
        QCOMPARE(rect->color(),QColor("yellow"));

        rectPrivate->setState("green");
        QCOMPARE(rect->color(),QColor("green"));
        rect->setProperty("sourceColor", QColor("purple"));
        QCOMPARE(rect->color(),QColor("green"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("purple"));

        rectPrivate->setState("green");
        QCOMPARE(rect->color(),QColor("green"));

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("red"));
    }
}

void tst_qquickstates::signalOverride()
{
    QQmlEngine engine;

    {
        QQmlComponent rectComponent(&engine, testFileUrl("signalOverride.qml"));
        QScopedPointer<MyRect> rect(qobject_cast<MyRect*>(rectComponent.create()));
        QVERIFY(rect != nullptr);

        QCOMPARE(rect->color(),QColor("red"));
        rect->doSomething();
        QCOMPARE(rect->color(),QColor("blue"));

        QQuickItemPrivate::get(rect.get())->setState("green");
        rect->doSomething();
        QCOMPARE(rect->color(),QColor("green"));
    }

    {
        QQmlComponent rectComponent(&engine, testFileUrl("signalOverride2.qml"));
        QScopedPointer<MyRect>rect(qobject_cast<MyRect*>(rectComponent.create()));
        QVERIFY(rect != nullptr);

        QCOMPARE(rect->color(),QColor("white"));
        rect->doSomething();
        QCOMPARE(rect->color(),QColor("blue"));

        QQuickRectangle *innerRect = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("extendedRect"));
        QQuickItemPrivate::get(innerRect)->setState("green");
        rect->doSomething();
        QCOMPARE(rect->color(),QColor("blue"));
        QCOMPARE(innerRect->color(),QColor("green"));
        QCOMPARE(innerRect->property("extendedColor").value<QColor>(),QColor("green"));
    }
}

void tst_qquickstates::signalOverrideCrash()
{
    QQmlEngine engine;

    QQmlComponent rectComponent(&engine, testFileUrl("signalOverrideCrash.qml"));
    QScopedPointer<MyRect> rect(qobject_cast<MyRect*>(rectComponent.create()));
    QVERIFY(rect != nullptr);

    QQuickItemPrivate::get(rect.get())->setState("overridden");
    rect->doSomething();
}

void tst_qquickstates::signalOverrideCrash2()
{
    QQmlEngine engine;

    QQmlComponent rectComponent(&engine, testFileUrl("signalOverrideCrash2.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
    QVERIFY(rect != nullptr);

    auto priv = QQuickItemPrivate::get(rect.get());
    priv->setState("state1");
    priv->setState("state2");
    priv->setState("state1");
}

void tst_qquickstates::signalOverrideCrash3()
{
    QQmlEngine engine;

    QQmlComponent rectComponent(&engine, testFileUrl("signalOverrideCrash3.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
    QVERIFY(rect != nullptr);

    auto priv = QQuickItemPrivate::get(rect.get());
    priv->setState("state1");
    priv->setState("");
    priv->setState("state2");
    priv->setState("");
}

void tst_qquickstates::signalOverrideCrash4()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("signalOverrideCrash4.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));
    QVERIFY(rect != nullptr);

    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());

    rectPrivate->setState("state1");
    rectPrivate->setState("state2");
    rectPrivate->setState("state1");
    rectPrivate->setState("state2");
    rectPrivate->setState("");
}

void tst_qquickstates::parentChange()
{
    QQmlEngine engine;

    {
        QQmlComponent rectComponent(&engine, testFileUrl("parentChange1.qml"));
        QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
        QVERIFY(rect != nullptr);

        QQuickRectangle *innerRect = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"));
        QVERIFY(innerRect != nullptr);

        QQmlListReference list(rect.get(), "states");
        QQuickState *state = qobject_cast<QQuickState*>(list.at(0));
        QVERIFY(state != nullptr);

        qmlExecuteDeferred(state);
        QQuickParentChange *pChange = qobject_cast<QQuickParentChange*>(state->operationAt(0));
        QVERIFY(pChange != nullptr);
        QQuickItem *nParent = qobject_cast<QQuickItem*>(rect->findChild<QQuickItem*>("NewParent"));
        QVERIFY(nParent != nullptr);

        QCOMPARE(pChange->parent(), nParent);

        QQuickItemPrivate::get(rect.get())->setState("reparented");
        QCOMPARE(innerRect->rotation(), qreal(0));
        QCOMPARE(innerRect->scale(), qreal(1));
        QCOMPARE(innerRect->x(), qreal(-133));
        QCOMPARE(innerRect->y(), qreal(-300));
    }

    {
        QQmlComponent rectComponent(&engine, testFileUrl("parentChange2.qml"));
        QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
        QVERIFY(rect != nullptr);
        QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());
        QQuickRectangle *innerRect = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"));
        QVERIFY(innerRect != nullptr);

        rectPrivate->setState("reparented");
        QCOMPARE(innerRect->rotation(), qreal(15));
        QCOMPARE(innerRect->scale(), qreal(.5));
        QCOMPARE(QString("%1").arg(innerRect->x()), QString("%1").arg(-19.9075));
        QCOMPARE(QString("%1").arg(innerRect->y()), QString("%1").arg(-8.73433));
    }

    {
        QQmlComponent rectComponent(&engine, testFileUrl("parentChange3.qml"));
        QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
        QVERIFY(rect != nullptr);
        QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());
        QQuickRectangle *innerRect = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"));
        QVERIFY(innerRect != nullptr);

        rectPrivate->setState("reparented");
        QCOMPARE(innerRect->rotation(), qreal(-37));
        QCOMPARE(innerRect->scale(), qreal(.25));
        QCOMPARE(QString("%1").arg(innerRect->x()), QString("%1").arg(-217.305));
        QCOMPARE(QString("%1").arg(innerRect->y()), QString("%1").arg(-164.413));

        rectPrivate->setState("");
        QCOMPARE(innerRect->rotation(), qreal(0));
        QCOMPARE(innerRect->scale(), qreal(1));
        QCOMPARE(innerRect->x(), qreal(5));
        //do a non-qFuzzyCompare fuzzy compare
        QVERIFY(innerRect->y() < qreal(0.00001) && innerRect->y() > qreal(-0.00001));
    }

    {
        QQmlComponent rectComponent(&engine, testFileUrl("parentChange6.qml"));
        QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
        QVERIFY(rect != nullptr);

        QQuickRectangle *innerRect = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"));
        QVERIFY(innerRect != nullptr);

        QQuickItemPrivate::get(rect.get())->setState("reparented");
        QCOMPARE(innerRect->rotation(), qreal(180));
        QCOMPARE(innerRect->scale(), qreal(1));
        QCOMPARE(innerRect->x(), qreal(-105));
        QCOMPARE(innerRect->y(), qreal(-105));
    }
}

void tst_qquickstates::parentChangeErrors()
{
    QQmlEngine engine;

    {
        QQmlComponent rectComponent(&engine, testFileUrl("parentChange4.qml"));
        QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
        QVERIFY(rect != nullptr);

        QQuickRectangle *innerRect = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"));
        QVERIFY(innerRect != nullptr);

        QTest::ignoreMessage(QtWarningMsg, fullDataPath("parentChange4.qml") + ":25:9: QML ParentChange: Unable to preserve appearance under non-uniform scale");
        QQuickItemPrivate::get(rect.get())->setState("reparented");
        QCOMPARE(innerRect->rotation(), qreal(0));
        QCOMPARE(innerRect->scale(), qreal(1));
        QCOMPARE(innerRect->x(), qreal(5));
        QCOMPARE(innerRect->y(), qreal(5));
    }

    {
        QQmlComponent rectComponent(&engine, testFileUrl("parentChange5.qml"));
        QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
        QVERIFY(rect != nullptr);

        QQuickRectangle *innerRect = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"));
        QVERIFY(innerRect != nullptr);

        QTest::ignoreMessage(QtWarningMsg, fullDataPath("parentChange5.qml") + ":25:9: QML ParentChange: Unable to preserve appearance under complex transform");
        QQuickItemPrivate::get(rect.get())->setState("reparented");
        QCOMPARE(innerRect->rotation(), qreal(0));
        QCOMPARE(innerRect->scale(), qreal(1));
        QCOMPARE(innerRect->x(), qreal(5));
        QCOMPARE(innerRect->y(), qreal(5));
    }
}

void tst_qquickstates::anchorChanges()
{
    QQmlEngine engine;

    QQmlComponent rectComponent(&engine, testFileUrl("anchorChanges1.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
    QVERIFY(rect != nullptr);
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());

    QQuickRectangle *innerRect = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"));
    QVERIFY(innerRect != nullptr);

    QQmlListReference list(rect.get(), "states");
    QQuickState *state = qobject_cast<QQuickState*>(list.at(0));
    QVERIFY(state != nullptr);

    qmlExecuteDeferred(state);
    QQuickAnchorChanges *aChanges = qobject_cast<QQuickAnchorChanges*>(state->operationAt(0));
    QVERIFY(aChanges != nullptr);

    QCOMPARE(aChanges->anchors()->left().isUndefinedLiteral(), true);
    QVERIFY(!aChanges->anchors()->left().isEmpty());
    QVERIFY(!aChanges->anchors()->right().isEmpty());

    rectPrivate->setState("right");
    QCOMPARE(innerRect->x(), qreal(150));
    QCOMPARE(aChanges->object(), qobject_cast<QQuickItem*>(innerRect));
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->left().anchorLine, QQuickAnchors::InvalidAnchor);  //### was reset (how do we distinguish from not set at all)
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->right().item, rectPrivate->right().item);
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->right().anchorLine, rectPrivate->right().anchorLine);

    rectPrivate->setState("");
    QCOMPARE(innerRect->x(), qreal(5));
}

void tst_qquickstates::anchorChanges2()
{
    QQmlEngine engine;

    QQmlComponent rectComponent(&engine, testFileUrl("anchorChanges2.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
    QVERIFY(rect != nullptr);
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());

    QQuickRectangle *innerRect = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"));
    QVERIFY(innerRect != nullptr);

    rectPrivate->setState("right");
    QCOMPARE(innerRect->x(), qreal(150));

    rectPrivate->setState("");
    QCOMPARE(innerRect->x(), qreal(5));
}

void tst_qquickstates::anchorChanges3()
{
    QQmlEngine engine;

    QQmlComponent rectComponent(&engine, testFileUrl("anchorChanges3.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
    QVERIFY(rect != nullptr);
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());

    QQuickRectangle *innerRect = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"));
    QVERIFY(innerRect != nullptr);

    QQuickItem *leftGuideline = qobject_cast<QQuickItem*>(rect->findChild<QQuickItem*>("LeftGuideline"));
    QVERIFY(leftGuideline != nullptr);

    QQuickItem *bottomGuideline = qobject_cast<QQuickItem*>(rect->findChild<QQuickItem*>("BottomGuideline"));
    QVERIFY(bottomGuideline != nullptr);

    QQmlListReference list(rect.get(), "states");
    QQuickState *state = qobject_cast<QQuickState*>(list.at(0));
    QVERIFY(state != nullptr);

    qmlExecuteDeferred(state);
    QQuickAnchorChanges *aChanges = qobject_cast<QQuickAnchorChanges*>(state->operationAt(0));
    QVERIFY(aChanges != nullptr);

    QVERIFY(!aChanges->anchors()->top().isEmpty());
    QVERIFY(!aChanges->anchors()->bottom().isEmpty());

    rectPrivate->setState("reanchored");
    QCOMPARE(aChanges->object(), qobject_cast<QQuickItem*>(innerRect));
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->left().item, QQuickItemPrivate::get(leftGuideline)->left().item);
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->left().anchorLine, QQuickItemPrivate::get(leftGuideline)->left().anchorLine);
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->right().item, rectPrivate->right().item);
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->right().anchorLine, rectPrivate->right().anchorLine);
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->top().item, rectPrivate->top().item);
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->top().anchorLine, rectPrivate->top().anchorLine);
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->bottom().item, QQuickItemPrivate::get(bottomGuideline)->bottom().item);
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->bottom().anchorLine, QQuickItemPrivate::get(bottomGuideline)->bottom().anchorLine);

    QCOMPARE(innerRect->x(), qreal(10));
    QCOMPARE(innerRect->y(), qreal(0));
    QCOMPARE(innerRect->width(), qreal(190));
    QCOMPARE(innerRect->height(), qreal(150));

    rectPrivate->setState("");
    QCOMPARE(innerRect->x(), qreal(0));
    QCOMPARE(innerRect->y(), qreal(10));
    QCOMPARE(innerRect->width(), qreal(150));
    QCOMPARE(innerRect->height(), qreal(190));
}

void tst_qquickstates::anchorChanges4()
{
    QQmlEngine engine;

    QQmlComponent rectComponent(&engine, testFileUrl("anchorChanges4.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
    QVERIFY(rect != nullptr);

    QQuickRectangle *innerRect = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"));
    QVERIFY(innerRect != nullptr);

    QQuickItem *leftGuideline = qobject_cast<QQuickItem*>(rect->findChild<QQuickItem*>("LeftGuideline"));
    QVERIFY(leftGuideline != nullptr);

    QQuickItem *bottomGuideline = qobject_cast<QQuickItem*>(rect->findChild<QQuickItem*>("BottomGuideline"));
    QVERIFY(bottomGuideline != nullptr);

    QQmlListReference list(rect.get(), "states");
    QQuickState *state = qobject_cast<QQuickState*>(list.at(0));
    QVERIFY(state != nullptr);

    qmlExecuteDeferred(state);
    QQuickAnchorChanges *aChanges = qobject_cast<QQuickAnchorChanges*>(state->operationAt(0));
    QVERIFY(aChanges != nullptr);

    QVERIFY(!aChanges->anchors()->horizontalCenter().isEmpty());
    QVERIFY(!aChanges->anchors()->verticalCenter().isEmpty());

    QQuickItemPrivate::get(rect.get())->setState("reanchored");
    QCOMPARE(aChanges->object(), qobject_cast<QQuickItem*>(innerRect));
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->horizontalCenter().item, QQuickItemPrivate::get(bottomGuideline)->horizontalCenter().item);
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->horizontalCenter().anchorLine, QQuickItemPrivate::get(bottomGuideline)->horizontalCenter().anchorLine);
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->verticalCenter().item, QQuickItemPrivate::get(leftGuideline)->verticalCenter().item);
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->verticalCenter().anchorLine, QQuickItemPrivate::get(leftGuideline)->verticalCenter().anchorLine);
}

void tst_qquickstates::anchorChanges5()
{
    QQmlEngine engine;

    QQmlComponent rectComponent(&engine, testFileUrl("anchorChanges5.qml"));
    QScopedPointer<QQuickRectangle>rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
    QVERIFY(rect != nullptr);

    QQuickRectangle *innerRect = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"));
    QVERIFY(innerRect != nullptr);

    QQuickItem *leftGuideline = qobject_cast<QQuickItem*>(rect->findChild<QQuickItem*>("LeftGuideline"));
    QVERIFY(leftGuideline != nullptr);

    QQuickItem *bottomGuideline = qobject_cast<QQuickItem*>(rect->findChild<QQuickItem*>("BottomGuideline"));
    QVERIFY(bottomGuideline != nullptr);

    QQmlListReference list(rect.get(), "states");
    QQuickState *state = qobject_cast<QQuickState*>(list.at(0));
    QVERIFY(state != nullptr);

    qmlExecuteDeferred(state);
    QQuickAnchorChanges *aChanges = qobject_cast<QQuickAnchorChanges*>(state->operationAt(0));
    QVERIFY(aChanges != nullptr);

    QVERIFY(!aChanges->anchors()->baseline().isEmpty());

    QQuickItemPrivate::get(rect.get())->setState("reanchored");
    QCOMPARE(aChanges->object(), qobject_cast<QQuickItem*>(innerRect));
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->horizontalCenter().item, QQuickItemPrivate::get(bottomGuideline)->horizontalCenter().item);
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->horizontalCenter().anchorLine, QQuickItemPrivate::get(bottomGuideline)->horizontalCenter().anchorLine);
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->baseline().item, QQuickItemPrivate::get(leftGuideline)->baseline().item);
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->baseline().anchorLine, QQuickItemPrivate::get(leftGuideline)->baseline().anchorLine);
}

void mirrorAnchors(QQuickItem *item) {
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
    itemPrivate->setLayoutMirror(true);
}

qreal offsetRTL(QQuickItem *anchorItem, QQuickItem *item) {
    return anchorItem->width()+2*anchorItem->x()-item->width();
}

void tst_qquickstates::anchorChangesRTL()
{
    QQmlEngine engine;

    QQmlComponent rectComponent(&engine, testFileUrl("anchorChanges1.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
    QVERIFY(rect != nullptr);
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());

    QQuickRectangle *innerRect = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"));
    QVERIFY(innerRect != nullptr);
    mirrorAnchors(innerRect);

    QQmlListReference list(rect.get(), "states");
    QQuickState *state = qobject_cast<QQuickState*>(list.at(0));
    QVERIFY(state != nullptr);

    qmlExecuteDeferred(state);
    QQuickAnchorChanges *aChanges = qobject_cast<QQuickAnchorChanges*>(state->operationAt(0));
    QVERIFY(aChanges != nullptr);

    rectPrivate->setState("right");
    QCOMPARE(innerRect->x(), offsetRTL(rect.get(), innerRect) - qreal(150));
    QCOMPARE(aChanges->object(), qobject_cast<QQuickItem*>(innerRect));
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->left().anchorLine, QQuickAnchors::InvalidAnchor);  //### was reset (how do we distinguish from not set at all)
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->right().item, rectPrivate->right().item);
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->right().anchorLine, rectPrivate->right().anchorLine);

    rectPrivate->setState("");
    QCOMPARE(innerRect->x(), offsetRTL(rect.get(), innerRect) -qreal(5));
}

void tst_qquickstates::anchorChangesRTL2()
{
    QQmlEngine engine;

    QQmlComponent rectComponent(&engine, testFileUrl("anchorChanges2.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
    QVERIFY(rect != nullptr);
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());

    QQuickRectangle *innerRect = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"));
    QVERIFY(innerRect != nullptr);
    mirrorAnchors(innerRect);

    rectPrivate->setState("right");
    QCOMPARE(innerRect->x(), offsetRTL(rect.get(), innerRect) - qreal(150));

    rectPrivate->setState("");
    QCOMPARE(innerRect->x(), offsetRTL(rect.get(), innerRect) - qreal(5));
}

void tst_qquickstates::anchorChangesRTL3()
{
    QQmlEngine engine;

    QQmlComponent rectComponent(&engine, testFileUrl("anchorChanges3.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
    QVERIFY(rect != nullptr);
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());

    QQuickRectangle *innerRect = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("MyRect"));
    QVERIFY(innerRect != nullptr);
    mirrorAnchors(innerRect);

    QQuickItem *leftGuideline = qobject_cast<QQuickItem*>(rect->findChild<QQuickItem*>("LeftGuideline"));
    QVERIFY(leftGuideline != nullptr);

    QQuickItem *bottomGuideline = qobject_cast<QQuickItem*>(rect->findChild<QQuickItem*>("BottomGuideline"));
    QVERIFY(bottomGuideline != nullptr);

    QQmlListReference list(rect.get(), "states");
    QQuickState *state = qobject_cast<QQuickState*>(list.at(0));
    QVERIFY(state != nullptr);

    qmlExecuteDeferred(state);
    QQuickAnchorChanges *aChanges = qobject_cast<QQuickAnchorChanges*>(state->operationAt(0));
    QVERIFY(aChanges != nullptr);

    rectPrivate->setState("reanchored");
    QCOMPARE(aChanges->object(), qobject_cast<QQuickItem*>(innerRect));
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->left().item, QQuickItemPrivate::get(leftGuideline)->left().item);
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->left().anchorLine, QQuickItemPrivate::get(leftGuideline)->left().anchorLine);
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->right().item, rectPrivate->right().item);
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->right().anchorLine, rectPrivate->right().anchorLine);
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->top().item, rectPrivate->top().item);
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->top().anchorLine, rectPrivate->top().anchorLine);
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->bottom().item, QQuickItemPrivate::get(bottomGuideline)->bottom().item);
    QCOMPARE(QQuickItemPrivate::get(aChanges->object())->anchors()->bottom().anchorLine, QQuickItemPrivate::get(bottomGuideline)->bottom().anchorLine);

    QCOMPARE(innerRect->x(), offsetRTL(leftGuideline, innerRect) - qreal(10));
    QCOMPARE(innerRect->y(), qreal(0));
    // between left side of parent and leftGuideline.x: 10, which has width 0
    QCOMPARE(innerRect->width(), qreal(10));
    QCOMPARE(innerRect->height(), qreal(150));

    rectPrivate->setState("");
    QCOMPARE(innerRect->x(), offsetRTL(rect.get(), innerRect) - qreal(0));
    QCOMPARE(innerRect->y(), qreal(10));
    // between right side of parent and left side of rightGuideline.x: 150, which has width 0
    QCOMPARE(innerRect->width(), qreal(50));
    QCOMPARE(innerRect->height(), qreal(190));
}

//QTBUG-9609
void tst_qquickstates::anchorChangesCrash()
{
    QQmlEngine engine;

    QQmlComponent rectComponent(&engine, testFileUrl("anchorChangesCrash.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
    QVERIFY(rect != nullptr);

    QQuickItemPrivate::get(rect.get())->setState("reanchored");
}

// QTBUG-12273
void tst_qquickstates::anchorRewindBug()
{
    QScopedPointer<QQuickView> view(new QQuickView);
    view->setSource(testFileUrl("anchorRewindBug.qml"));

    view->show();

    QVERIFY(QTest::qWaitForWindowExposed(view.get()));

    QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(view->rootObject());
    QVERIFY(rect != nullptr);

    QQuickItem * column = rect->findChild<QQuickItem*>("column");

    QVERIFY(column != nullptr);
    QVERIFY(!QQuickItemPrivate::get(column)->heightValid());
    QVERIFY(!QQuickItemPrivate::get(column)->widthValid());
    QCOMPARE(column->height(), 200.0);
    QQuickItemPrivate::get(rect)->setState("reanchored");

    // column height and width should stay implicit
    // and column's implicit resizing should still work
    QVERIFY(!QQuickItemPrivate::get(column)->heightValid());
    QVERIFY(!QQuickItemPrivate::get(column)->widthValid());
    QTRY_COMPARE(column->height(), 100.0);

    QQuickItemPrivate::get(rect)->setState("");

    // column height and width should stay implicit
    // and column's implicit resizing should still work
    QVERIFY(!QQuickItemPrivate::get(column)->heightValid());
    QVERIFY(!QQuickItemPrivate::get(column)->widthValid());
    QTRY_COMPARE(column->height(), 200.0);
}

// QTBUG-11834
void tst_qquickstates::anchorRewindBug2()
{
    QQmlEngine engine;

    QQmlComponent rectComponent(&engine, testFileUrl("anchorRewindBug2.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
    QVERIFY(rect != nullptr);

    QQuickRectangle *mover = rect->findChild<QQuickRectangle*>("mover");

    QVERIFY(mover != nullptr);
    QCOMPARE(mover->y(), qreal(0.0));
    QCOMPARE(mover->width(), qreal(50.0));

    QQuickItemPrivate::get(rect.get())->setState("anchored");
    QCOMPARE(mover->y(), qreal(250.0));
    QCOMPARE(mover->width(), qreal(200.0));

    QQuickItemPrivate::get(rect.get())->setState("");
    QCOMPARE(mover->y(), qreal(0.0));
    QCOMPARE(mover->width(), qreal(50.0));
}

void tst_qquickstates::script()
{
    QQmlEngine engine;

    {
        QQmlComponent rectComponent(&engine, testFileUrl("script.qml"));
        QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
        QVERIFY(rect != nullptr);
        QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());
        QCOMPARE(rect->color(),QColor("red"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("blue")); // a script isn't reverted
    }
}

void tst_qquickstates::restoreEntryValues()
{
    QQmlEngine engine;

    QQmlComponent rectComponent(&engine, testFileUrl("restoreEntryValues.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
    QVERIFY(rect != nullptr);
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());
    QCOMPARE(rect->color(),QColor("red"));

    rectPrivate->setState("blue");
    QCOMPARE(rect->color(),QColor("blue"));

    rectPrivate->setState("");
    QCOMPARE(rect->color(),QColor("blue"));
}

void tst_qquickstates::explicitChanges()
{
    QQmlEngine engine;

    QQmlComponent rectComponent(&engine, testFileUrl("explicit.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
    QVERIFY(rect != nullptr);
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());
    QQmlListReference list(rect.get(), "states");
    QQuickState *state = qobject_cast<QQuickState*>(list.at(0));
    QVERIFY(state != nullptr);

    qmlExecuteDeferred(state);
    QQuickPropertyChanges *changes = qobject_cast<QQuickPropertyChanges*>(rect->findChild<QQuickPropertyChanges*>("changes"));
    QVERIFY(changes != nullptr);
    QVERIFY(changes->isExplicit());

    QCOMPARE(rect->color(),QColor("red"));

    rectPrivate->setState("blue");
    QCOMPARE(rect->color(),QColor("blue"));

    rect->setProperty("sourceColor", QColor("green"));
    QCOMPARE(rect->color(),QColor("blue"));

    rectPrivate->setState("");
    QCOMPARE(rect->color(),QColor("red"));
    rect->setProperty("sourceColor", QColor("yellow"));
    QCOMPARE(rect->color(),QColor("red"));

    rectPrivate->setState("blue");
    QCOMPARE(rect->color(),QColor("yellow"));
}

void tst_qquickstates::propertyErrors()
{
    QQmlEngine engine;
    QQmlComponent rectComponent(&engine, testFileUrl("propertyErrors.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
    QVERIFY(rect != nullptr);

    QCOMPARE(rect->color(),QColor("red"));

    QTest::ignoreMessage(QtWarningMsg, fullDataPath("propertyErrors.qml") + ":8:9: QML PropertyChanges: Cannot assign to non-existent property \"colr\"");
    QTest::ignoreMessage(QtWarningMsg, fullDataPath("propertyErrors.qml") + ":8:9: QML PropertyChanges: Cannot assign to read-only property \"activeFocus\"");
    QQuickItemPrivate::get(rect.get())->setState("blue");
}

void tst_qquickstates::incorrectRestoreBug()
{
    QQmlEngine engine;

    QQmlComponent rectComponent(&engine, testFileUrl("basicChanges.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
    QVERIFY(rect != nullptr);
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());
    QCOMPARE(rect->color(),QColor("red"));

    rectPrivate->setState("blue");
    QCOMPARE(rect->color(),QColor("blue"));

    rectPrivate->setState("");
    QCOMPARE(rect->color(),QColor("red"));

    // make sure if we change the base state value, we then restore to it correctly
    rect->setColor(QColor("green"));

    rectPrivate->setState("blue");
    QCOMPARE(rect->color(),QColor("blue"));

    rectPrivate->setState("");
    QCOMPARE(rect->color(),QColor("green"));
}

void tst_qquickstates::autoStateAtStartupRestoreBug()
{
    QQmlEngine engine;

    QQmlComponent component(&engine, testFileUrl("autoStateAtStartupRestoreBug.qml"));
    QScopedPointer<QObject> obj(component.create());

    QVERIFY(obj != nullptr);
    QCOMPARE(obj->property("test").toInt(), 3);

    obj->setProperty("input", 2);

    QCOMPARE(obj->property("test").toInt(), 9);
}

void tst_qquickstates::deletingChange()
{
    QQmlEngine engine;

    QQmlComponent rectComponent(&engine, testFileUrl("deleting.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
    QVERIFY(rect != nullptr);
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());
    rectPrivate->setState("blue");
    QCOMPARE(rect->color(),QColor("blue"));
    QCOMPARE(rect->radius(),qreal(5));

    rectPrivate->setState("");
    QCOMPARE(rect->color(),QColor("red"));
    QCOMPARE(rect->radius(),qreal(0));

    QQuickPropertyChanges *pc = rect->findChild<QQuickPropertyChanges*>("pc1");
    QVERIFY(pc != nullptr);
    delete pc;

    QQuickState *state = rect->findChild<QQuickState*>();
    QVERIFY(state != nullptr);
    qmlExecuteDeferred(state);
    QCOMPARE(state->operationCount(), 1);

    rectPrivate->setState("blue");
    QCOMPARE(rect->color(),QColor("red"));
    QCOMPARE(rect->radius(),qreal(5));
}

void tst_qquickstates::deletingState()
{
    QQmlEngine engine;

    QQmlComponent rectComponent(&engine, testFileUrl("deletingState.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
    QVERIFY(rect != nullptr);

    QQuickStateGroup *sg = rect->findChild<QQuickStateGroup*>();
    QVERIFY(sg != nullptr);
    QVERIFY(sg->findState("blue") != nullptr);

    sg->setState("blue");
    QCOMPARE(rect->color(),QColor("blue"));

    sg->setState("");
    QCOMPARE(rect->color(),QColor("red"));

    QQuickState *state = rect->findChild<QQuickState*>();
    QVERIFY(state != nullptr);
    delete state;

    QVERIFY(!sg->findState("blue"));

    //### should we warn that state doesn't exist
    sg->setState("blue");
    QCOMPARE(rect->color(),QColor("red"));
}

void tst_qquickstates::tempState()
{
    QQmlEngine engine;

    QQmlComponent rectComponent(&engine, testFileUrl("legalTempState.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
    QVERIFY(rect != nullptr);
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());
    QTest::ignoreMessage(QtDebugMsg, "entering placed");
    QTest::ignoreMessage(QtDebugMsg, "entering idle");
    rectPrivate->setState("placed");
    QCOMPARE(rectPrivate->state(), QLatin1String("idle"));
}

void tst_qquickstates::illegalTempState()
{
    QQmlEngine engine;

    QQmlComponent rectComponent(&engine, testFileUrl("illegalTempState.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
    QVERIFY(rect != nullptr);
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());
    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML StateGroup: Can't apply a state change as part of a state definition.");
    rectPrivate->setState("placed");
    QCOMPARE(rectPrivate->state(), QLatin1String("placed"));
}

void tst_qquickstates::nonExistantProperty()
{
    QQmlEngine engine;

    QQmlComponent rectComponent(&engine, testFileUrl("nonExistantProp.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(rectComponent.create()));
    QVERIFY(rect != nullptr);
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());
    QTest::ignoreMessage(QtWarningMsg, fullDataPath("nonExistantProp.qml") + ":9:9: QML PropertyChanges: Cannot assign to non-existent property \"colr\"");
    rectPrivate->setState("blue");
    QCOMPARE(rectPrivate->state(), QLatin1String("blue"));
}

void tst_qquickstates::reset()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("reset.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));
    QVERIFY(rect != nullptr);

    QQuickImage *image = rect->findChild<QQuickImage*>();
    QVERIFY(image != nullptr);
    QCOMPARE(image->width(), qreal(40.));
    QCOMPARE(image->height(), qreal(20.));

    QQuickItemPrivate::get(rect.get())->setState("state1");

    QCOMPARE(image->width(), 20.0);
    QCOMPARE(image->height(), qreal(20.));
}

void tst_qquickstates::illegalObjectCreation()
{
    QQmlEngine engine;

    QQmlComponent component(&engine, testFileUrl("illegalObj.qml"));
    QList<QQmlError> errors = component.errors();
    QCOMPARE(errors.size(), 1);
    const QQmlError &error = errors.at(0);
    QCOMPARE(error.line(), 9);
    QCOMPARE(error.column(), 23);
    QCOMPARE(error.description().toUtf8().constData(), "PropertyChanges does not support creating state-specific objects.");
}

void tst_qquickstates::whenOrdering()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("whenOrdering.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));
    QVERIFY(rect != nullptr);
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());

    QCOMPARE(rectPrivate->state(), QLatin1String(""));
    rect->setProperty("condition2", true);
    QCOMPARE(rectPrivate->state(), QLatin1String("state2"));
    rect->setProperty("condition1", true);
    QCOMPARE(rectPrivate->state(), QLatin1String("state1"));
    rect->setProperty("condition2", false);
    QCOMPARE(rectPrivate->state(), QLatin1String("state1"));
    rect->setProperty("condition2", true);
    QCOMPARE(rectPrivate->state(), QLatin1String("state1"));
    rect->setProperty("condition1", false);
    rect->setProperty("condition2", false);
    QCOMPARE(rectPrivate->state(), QLatin1String(""));
}

void tst_qquickstates::urlResolution()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("urlResolution.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));
    QVERIFY(rect != nullptr);

    QQuickItem *myType = rect->findChild<QQuickItem*>("MyType");
    QQuickImage *image1 = rect->findChild<QQuickImage*>("image1");
    QQuickImage *image2 = rect->findChild<QQuickImage*>("image2");
    QQuickImage *image3 = rect->findChild<QQuickImage*>("image3");
    QVERIFY(myType != nullptr && image1 != nullptr && image2 != nullptr && image3 != nullptr);

    QQuickItemPrivate::get(myType)->setState("SetImageState");
    QUrl resolved = QUrl("images/qt-logo.png");
    QCOMPARE(image1->source(), resolved);
    QCOMPARE(image2->source(), resolved);
    QCOMPARE(image3->source(), resolved);
}

void tst_qquickstates::unnamedWhen()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("unnamedWhen.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));
    QVERIFY(rect != nullptr);
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());

    QCOMPARE(rectPrivate->state(), QLatin1String(""));
    QCOMPARE(rect->property("stateString").toString(), QLatin1String(""));
    rect->setProperty("triggerState", true);
    QCOMPARE(rectPrivate->state(), QLatin1String("anonymousState1"));
    QCOMPARE(rect->property("stateString").toString(), QLatin1String("inState"));
    rect->setProperty("triggerState", false);
    QCOMPARE(rectPrivate->state(), QLatin1String(""));
    QCOMPARE(rect->property("stateString").toString(), QLatin1String(""));
}

void tst_qquickstates::returnToBase()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("returnToBase.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));
    QVERIFY(rect != nullptr);
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());

    QCOMPARE(rectPrivate->state(), QLatin1String(""));
    QCOMPARE(rect->property("stateString").toString(), QLatin1String(""));
    rect->setProperty("triggerState", true);
    QCOMPARE(rectPrivate->state(), QLatin1String("anonymousState1"));
    QCOMPARE(rect->property("stateString").toString(), QLatin1String("inState"));
    rect->setProperty("triggerState", false);
    QCOMPARE(rectPrivate->state(), QLatin1String(""));
    QCOMPARE(rect->property("stateString").toString(), QLatin1String("originalState"));
}

//QTBUG-12559
void tst_qquickstates::extendsBug()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("extendsBug.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));
    QVERIFY(rect != nullptr);
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());
    QQuickRectangle *greenRect = rect->findChild<QQuickRectangle*>("greenRect");

    rectPrivate->setState("b");
    QCOMPARE(greenRect->x(), qreal(100));
    QCOMPARE(greenRect->y(), qreal(100));
}

void tst_qquickstates::editProperties()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("editProperties.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));
    QVERIFY(rect != nullptr);

    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());

    QQuickStateGroup *stateGroup = rectPrivate->_states();
    QVERIFY(stateGroup != nullptr);
    qmlExecuteDeferred(stateGroup);

    QQuickState *blueState = stateGroup->findState("blue");
    QVERIFY(blueState != nullptr);
    qmlExecuteDeferred(blueState);

    QQuickPropertyChanges *propertyChangesBlue = qobject_cast<QQuickPropertyChanges*>(blueState->operationAt(0));
    QVERIFY(propertyChangesBlue != nullptr);

    QQuickState *greenState = stateGroup->findState("green");
    QVERIFY(greenState != nullptr);
    qmlExecuteDeferred(greenState);

    QQuickPropertyChanges *propertyChangesGreen = qobject_cast<QQuickPropertyChanges*>(greenState->operationAt(0));
    QVERIFY(propertyChangesGreen != nullptr);

    QQuickRectangle *childRect = rect->findChild<QQuickRectangle*>("rect2");
    QVERIFY(childRect != nullptr);
    QCOMPARE(childRect->width(), qreal(402));
    QVERIFY(QQmlAnyBinding::ofProperty(QQmlProperty(childRect, "width")));
    QCOMPARE(childRect->height(), qreal(200));

    rectPrivate->setState("blue");
    QCOMPARE(childRect->width(), qreal(50));
    QCOMPARE(childRect->height(), qreal(40));
    QVERIFY(!QQmlPropertyPrivate::binding(QQmlProperty(childRect, "width")));
    QVERIFY(blueState->bindingInRevertList(childRect, "width"));


    rectPrivate->setState("green");
    QCOMPARE(childRect->width(), qreal(200));
    QCOMPARE(childRect->height(), qreal(100));
    QVERIFY(greenState->bindingInRevertList(childRect, "width"));


    rectPrivate->setState("");


    QCOMPARE(propertyChangesBlue->actions().size(), 2);
    QVERIFY(propertyChangesBlue->containsValue("width"));
    QVERIFY(!propertyChangesBlue->containsProperty("x"));
    QCOMPARE(propertyChangesBlue->value("width").toInt(), 50);
    QVERIFY(!propertyChangesBlue->value("x").isValid());

    propertyChangesBlue->changeValue("width", 60);
    QCOMPARE(propertyChangesBlue->value("width").toInt(), 60);
    QCOMPARE(propertyChangesBlue->actions().size(), 2);


    propertyChangesBlue->changeExpression("width", "myRectangle.width / 2");
    QVERIFY(!propertyChangesBlue->containsValue("width"));
    QVERIFY(propertyChangesBlue->containsExpression("width"));
    QCOMPARE(propertyChangesBlue->value("width").toInt(), 0);
    QCOMPARE(propertyChangesBlue->actions().size(), 2);

    propertyChangesBlue->changeValue("width", 50);
    QVERIFY(propertyChangesBlue->containsValue("width"));
    QVERIFY(!propertyChangesBlue->containsExpression("width"));
    QCOMPARE(propertyChangesBlue->value("width").toInt(), 50);
    QCOMPARE(propertyChangesBlue->actions().size(), 2);

    QVERIFY(QQmlAnyBinding::ofProperty(QQmlProperty(childRect, "width")));
    rectPrivate->setState("blue");
    QCOMPARE(childRect->width(), qreal(50));
    QCOMPARE(childRect->height(), qreal(40));

    propertyChangesBlue->changeValue("width", 60);
    QCOMPARE(propertyChangesBlue->value("width").toInt(), 60);
    QCOMPARE(propertyChangesBlue->actions().size(), 2);
    QCOMPARE(childRect->width(), qreal(60));
    QVERIFY(!QQmlAnyBinding::ofProperty(QQmlProperty(childRect, "width")));

    propertyChangesBlue->changeExpression("width", "myRectangle.width / 2");
    QVERIFY(!propertyChangesBlue->containsValue("width"));
    QVERIFY(propertyChangesBlue->containsExpression("width"));
    QCOMPARE(propertyChangesBlue->value("width").toInt(), 0);
    QCOMPARE(propertyChangesBlue->actions().size(), 2);
    QVERIFY(QQmlAnyBinding::ofProperty(QQmlProperty(childRect, "width")));
    QCOMPARE(childRect->width(), qreal(200));

    propertyChangesBlue->changeValue("width", 50);
    QCOMPARE(childRect->width(), qreal(50));

    rectPrivate->setState("");
    QCOMPARE(childRect->width(), qreal(402));
    QVERIFY(QQmlAnyBinding::ofProperty(QQmlProperty(childRect, "width")));

    QCOMPARE(propertyChangesGreen->actions().size(), 2);
    rectPrivate->setState("green");
    QCOMPARE(childRect->width(), qreal(200));
    QCOMPARE(childRect->height(), qreal(100));
    QVERIFY(QQmlAnyBinding::ofProperty(QQmlProperty(childRect, "width")));
    QVERIFY(greenState->bindingInRevertList(childRect, "width"));
    QCOMPARE(propertyChangesGreen->actions().size(), 2);


    propertyChangesGreen->removeProperty("height");
    QVERIFY(!QQmlAnyBinding::ofProperty(QQmlProperty(childRect, "height")));
    QCOMPARE(childRect->height(), qreal(200));

    QVERIFY(greenState->bindingInRevertList(childRect, "width"));
    QVERIFY(greenState->containsPropertyInRevertList(childRect, "width"));
    propertyChangesGreen->removeProperty("width");
    QVERIFY(QQmlAnyBinding::ofProperty(QQmlProperty(childRect, "width")));
    QCOMPARE(childRect->width(), qreal(402));
    QVERIFY(!greenState->bindingInRevertList(childRect, "width"));
    QVERIFY(!greenState->containsPropertyInRevertList(childRect, "width"));

    propertyChangesBlue->removeProperty("width");
    QCOMPARE(childRect->width(), qreal(402));

    rectPrivate->setState("blue");
    QCOMPARE(childRect->width(), qreal(402));
    QCOMPARE(childRect->height(), qreal(40));
}

void tst_qquickstates::QTBUG_14830()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("QTBUG-14830.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));
    QVERIFY(rect != nullptr);
    QQuickItem *item = rect->findChild<QQuickItem*>("area");

    QCOMPARE(item->width(), qreal(170));
}

void tst_qquickstates::avoidFastForward()
{
    QQmlEngine engine;

    //shouldn't fast forward if there isn't a transition
    QQmlComponent c(&engine, testFileUrl("avoidFastForward.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));
    QVERIFY(rect != nullptr);

    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());
    rectPrivate->setState("a");
    QCOMPARE(rect->property("updateCount").toInt(), 1);
}

//QTBUG-22583
void tst_qquickstates::revertListBug()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("revertListBug.qml"));
    QScopedPointer<QQuickRectangle> rect(qobject_cast<QQuickRectangle*>(c.create()));
    QVERIFY(rect != nullptr);

    QQuickRectangle *rect1 = rect->findChild<QQuickRectangle*>("rect1");
    QQuickRectangle *rect2 = rect->findChild<QQuickRectangle*>("rect2");
    QQuickItem *origParent1 = rect->findChild<QQuickItem*>("originalParent1");
    QQuickItem *origParent2 = rect->findChild<QQuickItem*>("originalParent2");
    QQuickItem *newParent = rect->findChild<QQuickItem*>("newParent");

    QCOMPARE(rect1->parentItem(), origParent1);
    QCOMPARE(rect2->parentItem(), origParent2);

    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect.get());
    rectPrivate->setState("reparented");

    QCOMPARE(rect1->parentItem(), newParent);
    QCOMPARE(rect2->parentItem(), origParent2);

    rectPrivate->setState("");

    QCOMPARE(rect1->parentItem(), origParent1);
    QCOMPARE(rect2->parentItem(), origParent2);

    QMetaObject::invokeMethod(rect.get(), "switchTargetItem");

    rectPrivate->setState("reparented");

    QCOMPARE(rect1->parentItem(), origParent1);
    QCOMPARE(rect2->parentItem(), newParent);

    rectPrivate->setState("");

    QCOMPARE(rect1->parentItem(), origParent1);
    QCOMPARE(rect2->parentItem(), origParent2); //QTBUG-22583 causes rect2's parent item to be origParent1
}

void tst_qquickstates::QTBUG_38492()
{
    QQmlEngine engine;

    QQmlComponent rectComponent(&engine, testFileUrl("QTBUG-38492.qml"));
    QScopedPointer<QQuickItem> item(qobject_cast<QQuickItem*>(rectComponent.create()));
    QVERIFY(item != nullptr);

    QQuickItemPrivate::get(item.get())->setState("apply");

    QCOMPARE(item->property("text").toString(), QString("Test"));
}

static int getRefCount(const QQmlAnyBinding &binding)
{
    if (binding.isAbstractPropertyBinding()) {
        return binding.asAbstractBinding()->ref;
    } else {
        // this temporarily adds a refcount because we construc a new untypedpropertybinding
        // thus -1
        return QPropertyBindingPrivate::get(binding.asUntypedPropertyBinding())->ref - 1;
    }
}

void tst_qquickstates::revertListMemoryLeak()
{
    QQmlAnyBinding bindingPtr;
    {
        QQmlEngine engine;

        QQmlComponent c(&engine, testFileUrl("revertListMemoryLeak.qml"));
        QScopedPointer<QQuickItem> item(qobject_cast<QQuickItem *>(c.create()));
        QVERIFY(item);
        QQuickState *state = item->findChild<QQuickState*>("testState");
        QVERIFY(state);

        item->setState("testState");

        auto binding = state->bindingInRevertList(item.get(), "height");
        QVERIFY(binding);
        bindingPtr = binding;
        QVERIFY(getRefCount(bindingPtr) > 1);
    }
    QVERIFY(getRefCount(bindingPtr) == 1);
}

void tst_qquickstates::duplicateStateName()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("duplicateStateName.qml"));
    QTest::ignoreMessage(QtWarningMsg, fullDataPath("duplicateStateName.qml") + ":3:1: QML Rectangle: Found duplicate state name: state1");
    QScopedPointer<QQuickItem> item(qobject_cast<QQuickItem *>(c.create()));
    QVERIFY(!item.isNull());
}

// QTBUG-76838
void tst_qquickstates::trivialWhen()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("trivialWhen.qml"));
    QScopedPointer<QObject> root(c.create());
    QVERIFY(root);
}


void tst_qquickstates::jsValueWhen_data()
{
    QTest::addColumn<QByteArray>("fileName");
    QTest::addRow("jsObject") << QByteArray("jsValueWhen.qml");
    QTest::addRow("qmlObject") << QByteArray("jsValueWhen2.qml");
}

void tst_qquickstates::jsValueWhen()
{
    QFETCH(QByteArray, fileName);
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl(fileName.constData()));
    QScopedPointer<QObject> root(c.create());
    QVERIFY(root);
    QVERIFY(root->property("works").toBool());
}

void tst_qquickstates::noStateOsciallation()
{
   QQmlEngine engine;
   QQmlComponent component(&engine, testFileUrl("noStateOsciallation.qml"));
   QScopedPointer<QObject> root {component.create()};
   QVERIFY(root);
   // set to 1 on initial transition from "" to "n2"
   QCOMPARE(root->property("stateChangeCounter").toInt(), 1);
   root->setProperty("number", 1);
   // setting number to 1 changes directly from "n2" to "n1"
   // without any intermediate transition to ""
   QCOMPARE(root->property("stateChangeCounter").toInt(), 2);
}

void tst_qquickstates::parentChangeCorrectReversal()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("parentChangeCorrectReversal.qml"));
    QScopedPointer<QObject> root {c.create()};
    QVERIFY(root);
    QQmlProperty stayingRectX(root.get(), "stayingRectX");
    qreal oldX = stayingRectX.read().toDouble();
    QQmlProperty switchToRight(root.get(), "switchToRight");
    switchToRight.write(true);
    qreal newX = stayingRectX.read().toDouble();
    QVERIFY(newX != oldX);
    switchToRight.write(false);
    QCOMPARE(oldX, stayingRectX.read().toDouble());
}

void tst_qquickstates::revertNullObjectBinding()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, testFileUrl("revertNullObjectBinding.qml"));
    QScopedPointer<QObject> root { c.create() };
    QVERIFY(root);
    QTest::qWait(10);
    QQmlProperty state2Active(root.get(), "state2Active");
    state2Active.write(false);
}

void tst_qquickstates::bindableProperties()
{
    QQmlEngine engine;
    qmlRegisterType<MyBindable>("Qt.test", 1, 0, "MyBindable");
    QQmlComponent c(&engine, testFileUrl("bindableProperties.qml"));
    QScopedPointer<MyBindable> root { qobject_cast<MyBindable *>(c.create()) };
    QVERIFY(root);
    {
        // initial sanity check
        QCOMPARE(root->prop(), 84);
        QVERIFY(root->bindableProp().hasBinding());
        root->setX(0);
        QCOMPARE(root->prop(), 42);
    }
    {
        // When the state changes,
        root->setProperty("toggle", true);
        // the value gets updated,
        QCOMPARE(root->prop(), 5);
        // the binding is accessible from C++,
        QVERIFY(root->bindableProp().hasBinding());
        // and the new binding is active.
        root->setHeight(10);
        QCOMPARE(root->prop(), 10);
    }
    {
        // After changing the state back,
        root->setProperty("toggle", false);
        // the old value is restored,
        QCOMPARE(root->prop(), 42);
        // the binding is accessible from C++,
        QVERIFY(root->bindableProp().hasBinding());
        // and the old binding is active.
        root->setX(42);
        QCOMPARE(root->prop(), 84);
    }
}

struct Listener : QQuickItemChangeListener
{
    // We want to get notified about all the states.
    constexpr static const QRectF expectations[] = {
        QRectF(40, 40, 400, 400),
        QRectF(40, 0, 400, 400),
        QRectF(0, 0, 400, 400),
        QRectF(0, 0, 800, 400),
        QRectF(0, 0, 800, 200),
        QRectF(0, 0, 400, 200),
        QRectF(0, 20, 400, 200),
        QRectF(40, 20, 400, 200),
        QRectF(84, 42, 400, 200),
        QRectF(84, 42, 86, 43),
        QRectF(40, 40, 86, 43),
        QRectF(40, 40, 400, 400),
        QRectF(40, 20, 400, 400),
        QRectF(40, 20, 400, 200),
        QRectF(20, 20, 400, 200),
        QRectF(20, 20, 200, 200),
        QRectF(20, 20, 200, 300),
        QRectF(20, 20, 300, 300),
        QRectF(20, 30, 300, 300),
        QRectF(30, 30, 300, 300),
    };

    int position = 0;
    bool ok = true;

    void itemGeometryChanged(QQuickItem *, QQuickGeometryChange, const QRectF &rect) override
    {
        if (rect != expectations[position]) {
            qDebug() << position << rect;
            ok = false;
        }
        ++position;
    }
};

void tst_qquickstates::parentChangeInvolvingBindings()
{
   QQmlEngine engine;
   QQmlComponent c(&engine, testFileUrl("parentChangeInvolvingBindings.qml"));
   Listener listener;
   QScopedPointer<QQuickItem> root { qobject_cast<QQuickItem *>(c.create()) };
   QVERIFY2(root, qPrintable(c.errorString()));

   QObject *child = qmlContext(root.data())->objectForName(QStringLiteral("firstChild"));
   QVERIFY(child);
   QQuickItem *childItem = qobject_cast<QQuickItem *>(child);
   QVERIFY(childItem);
   QQuickItemPrivate::get(childItem)->addItemChangeListener(&listener, QQuickItemPrivate::Geometry);


   QCOMPARE(root->property("childWidth").toInt(), 400);
   QCOMPARE(root->property("childX").toInt(), 40);
   QCOMPARE(root->property("childRotation").toInt(), 100);
   root->setState("reparented");

   QCOMPARE(root->property("childWidth").toInt(), 800);
   QCOMPARE(root->property("childX").toInt(), 0); // x gets zeroed here, from unrelated place.
   QCOMPARE(root->property("childRotation").toInt(), 200);

   root->setProperty("myrotation2", 300);
   root->setHeight(200);
   root->setY(20);
   QCOMPARE(root->property("childRotation").toInt(), 300);
   QCOMPARE(root->property("childWidth").toInt(), 400);
   QCOMPARE(root->property("childX").toInt(), 40);

   QObject *inner = qmlContext(root.data())->objectForName(QStringLiteral("inner"));
   QVERIFY(inner);
   QQuickItem *innerItem = qobject_cast<QQuickItem *>(inner);
   QVERIFY(innerItem);

   QCOMPARE(innerItem->size(), childItem->size());

   // Does not break bindings and does not survive the state change.
   // However, since the binding between x and y stays intact, we don't know
   // whether x is set another time from the new y. We pass a pair of numbers that
   // matches the binding.
   childItem->setPosition(QPointF(84, 42));
   QCOMPARE(root->property("childX").toInt(), 84);
   QVERIFY(listener.ok);
   childItem->setSize(QSizeF(86, 43));
   QCOMPARE(root->property("childWidth").toInt(), 86);
   QVERIFY(listener.ok);

   QCOMPARE(innerItem->size(), childItem->size());

   QSignalSpy xSpy(childItem, SIGNAL(xChanged()));
   QSignalSpy widthSpy(childItem, SIGNAL(widthChanged()));

   root->setState("");

   QVERIFY(listener.ok);
   QCOMPARE(root->property("childRotation").toInt(), 100);

   // First change to 40 via reverse(), then to 20 via binding.
   QCOMPARE(xSpy.size(), 2);

   // First change to 400 via reverse(), then to 200 via binding.
   QCOMPARE(widthSpy.size(), 2);

   QCOMPARE(root->property("childX").toInt(), 20);
   QCOMPARE(root->property("childWidth").toInt(), 200);

   QCOMPARE(innerItem->size(), childItem->size());

   root->setProperty("myrotation", 50);
   root->setHeight(300);
   QVERIFY(listener.ok);
   root->setY(30);
   QVERIFY(listener.ok);
   QCOMPARE(root->property("childWidth").toInt(), 300);
   QCOMPARE(root->property("childX").toInt(), 30);
   QCOMPARE(root->property("childRotation").toInt(), 50);

   QCOMPARE(innerItem->size(), childItem->size());
}

void tst_qquickstates::deferredProperties()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("cleanPropertyChange.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QQuickRectangle> root(qobject_cast<QQuickRectangle *>(c.create()));
    QVERIFY(root);

    QCOMPARE(root->color(), QColor(Qt::red));
    QCOMPARE(qvariant_cast<QColor>(root->property("extendedColor")), QColor(Qt::cyan));
    QCOMPARE(root->width(), 100.0);
    QCOMPARE(root->height(), 100.0);

    QCOMPARE(root->state(), QString());
    root->setState(QStringLiteral("green"));

    QCOMPARE(root->color(), QColor(Qt::yellow));
    QCOMPARE(qvariant_cast<QColor>(root->property("extendedColor")), QColor(Qt::blue));
    QCOMPARE(root->width(), 90.0);
    QCOMPARE(root->height(), 90.0);

    QMetaObject::invokeMethod(root.get(), "didSomething");
    const QColor green = qRgb(0x00, 0x80, 0x00);
    QCOMPARE(root->color(), green);
    QCOMPARE(qvariant_cast<QColor>(root->property("extendedColor")), green);
    QCOMPARE(root->width(), 90.0);
    QCOMPARE(root->height(), 90.0);

    root->setState(QString());

    QCOMPARE(root->color(), QColor(Qt::red));
    QCOMPARE(qvariant_cast<QColor>(root->property("extendedColor")), QColor(Qt::cyan));
    QCOMPARE(root->width(), 100.0);
    QCOMPARE(root->height(), 100.0);
}

void tst_qquickstates::rewindAnchorChange()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("anchorRewind.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> root(c.create());
    QVERIFY(root);

    QQmlContext *context = qmlContext(root.data());
    QVERIFY(context);

    QObject *inner = context->objectForName(QStringLiteral("inner"));
    QVERIFY(inner);

    QQuickItem *innerRect = qobject_cast<QQuickItem *>(inner);
    QVERIFY(innerRect);

    QTRY_COMPARE(innerRect->x(), 0);
    QTRY_COMPARE(innerRect->y(), 0);
    QTRY_COMPARE(innerRect->width(), 200);
    QTRY_COMPARE(innerRect->height(), 200);
}

void tst_qquickstates::rewindAnchorChangeSize()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("anchorRewindSize.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    std::unique_ptr<QObject> root(c.create());
    QVERIFY(root);

    QQmlContext *context = qmlContext(root.get());
    QVERIFY(context);

    QObject *inner = context->objectForName(QStringLiteral("inner"));
    QVERIFY(inner);

    QQuickItem *innerRect = qobject_cast<QQuickItem *>(inner);
    QVERIFY(innerRect);

    QCOMPARE(innerRect->x(), 0);
    QCOMPARE(innerRect->y(), 0);
    QCOMPARE(innerRect->width(), 100);
    QCOMPARE(innerRect->height(), 100);

    root->setProperty("changeState", true);
    QCOMPARE(innerRect->x(), 0);
    QCOMPARE(innerRect->y(), 0);
    QCOMPARE(innerRect->width(), 400);
    QCOMPARE(innerRect->height(), 400);

    root->setProperty("changeState", false);
    QCOMPARE(innerRect->x(), 0);
    QCOMPARE(innerRect->y(), 0);
    QCOMPARE(innerRect->width(), 100);
    QCOMPARE(innerRect->height(), 100);
}

void tst_qquickstates::bindingProperlyRemovedWithTransition()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("removeBindingWithTransition.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> root(c.create());
    QVERIFY(root);
    QQuickItem *item = qobject_cast<QQuickItem *>(root.get());
    QVERIFY(item);

    item->setProperty("toggle", false);
    QTRY_COMPARE(item->width(), 300);

    item->setProperty("state1Width", 100);
    QCOMPARE(item->width(), 300);

    item->setProperty("toggle", true);
    QTRY_COMPARE(item->width(), 100);
}

QTEST_MAIN(tst_qquickstates)

#include "tst_qquickstates.moc"
