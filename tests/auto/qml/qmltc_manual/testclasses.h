// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TESTCLASSES_H
#define TESTCLASSES_H

#include <QtCore/qobject.h>
#include <QtCore/qproperty.h>
#include <QtQml/qqmlregistration.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickitem.h>

#include <private/qqmlrefcount_p.h>
#include <private/qqmlcontextdata_p.h>
#include <private/qqmlengine_p.h>

// utility class that sets up QQmlContext for passed QObject. can be used as a
// base class to ensure that qmlEngine(object) is valid during initializer list
// evaluation
struct ContextRegistrator
{
    ContextRegistrator(QQmlEngine *engine, QObject *This);

    static QQmlRefPointer<QQmlContextData>
    create(QQmlEngine *engine, const QUrl &url,
           const QQmlRefPointer<QQmlContextData> &parentContext, int index)
    {
        // test-specific wrapping that is document-root-agnostic. calls proper
        // creation for index == 0 and falls back to returning parentContext
        // otherwise. the qmltc generates exactly that but with an implicit
        // index check
        if (index == 0) {
            auto priv = QQmlEnginePrivate::get(engine);
            return priv->createInternalContext(priv->compilationUnitFromUrl(url), parentContext, 0,
                                               true);
        }
        return parentContext;
    }

    static void set(QObject *This, const QQmlRefPointer<QQmlContextData> &context,
                    QQmlContextData::QmlObjectKind kind)
    {
        QQmlEnginePrivate::setInternalContext(This, context, kind);
    }
};

class HelloWorld : public QObject, public ContextRegistrator
{
    Q_OBJECT
    QML_NAMED_ELEMENT(HelloWorld);
    QML_UNCREATABLE("")
    Q_PROPERTY(QString hello READ getHello WRITE setHello BINDABLE bindableHello)
    Q_PROPERTY(QString greeting READ getGreeting WRITE setGreeting BINDABLE bindableGreeting)

public:
    // test workaround: the url is resolved by the test base class, so use
    // member variable to store the resolved url used as argument in engine
    // evaluation of runtime functions
    static QUrl url;

    HelloWorld(QQmlEngine *e, QObject *parent = nullptr);
    virtual ~HelloWorld() { }

    QString getHello() { return hello.value(); }
    QString getGreeting() { return greeting.value(); }

    void setHello(const QString &hello_) { hello.setValue(hello_); }
    void setGreeting(const QString &greeting_) { greeting.setValue(greeting_); }

    QBindable<QString> bindableHello() { return QBindable<QString>(&hello); }
    QBindable<QString> bindableGreeting() { return QBindable<QString>(&greeting); }

    Q_OBJECT_BINDABLE_PROPERTY(HelloWorld, QString, hello);
    Q_OBJECT_BINDABLE_PROPERTY(HelloWorld, QString, greeting);
};

class ANON_signalHandlers : public QObject, public ContextRegistrator
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int signal1P READ getSignal1P WRITE setSignal1P BINDABLE bindableSignal1P)
    Q_PROPERTY(QString signal2P1 READ getSignal2P1 WRITE setSignal2P1 BINDABLE bindableSignal2P1)
    Q_PROPERTY(int signal2P2 READ getSignal2P2 WRITE setSignal2P2 BINDABLE bindableSignal2P2)
    Q_PROPERTY(QString signal2P3 READ getSignal2P3 WRITE setSignal2P3 BINDABLE bindableSignal2P3)

public:
    // test workaround: the url is resolved by the test base class, so use
    // member variable to store the resolved url used as argument in engine
    // evaluation of runtime functions
    static QUrl url;

    ANON_signalHandlers(QQmlEngine *e, QObject *parent = nullptr);

    int getSignal1P() { return signal1P.value(); }
    QString getSignal2P1() { return signal2P1.value(); }
    int getSignal2P2() { return signal2P2.value(); }
    QString getSignal2P3() { return signal2P3.value(); }

    void setSignal1P(const int &signal1P_) { signal1P.setValue(signal1P_); }
    void setSignal2P1(const QString &signal2P1_) { signal2P1.setValue(signal2P1_); }
    void setSignal2P2(const int &signal2P2_) { signal2P2.setValue(signal2P2_); }
    void setSignal2P3(const QString &signal2P3_) { signal2P3.setValue(signal2P3_); }

    QBindable<int> bindableSignal1P() { return QBindable<int>(&signal1P); }
    QBindable<QString> bindableSignal2P1() { return QBindable<QString>(&signal2P1); }
    QBindable<int> bindableSignal2P2() { return QBindable<int>(&signal2P2); }
    QBindable<QString> bindableSignal2P3() { return QBindable<QString>(&signal2P3); }

    Q_OBJECT_BINDABLE_PROPERTY(ANON_signalHandlers, int, signal1P);
    Q_OBJECT_BINDABLE_PROPERTY(ANON_signalHandlers, QString, signal2P1);
    Q_OBJECT_BINDABLE_PROPERTY(ANON_signalHandlers, int, signal2P2);
    Q_OBJECT_BINDABLE_PROPERTY(ANON_signalHandlers, QString, signal2P3);

Q_SIGNALS:
    void signal1();
    void signal2(QString x, int y);

public slots:
    void onSignal1();
    void onSignal2(QString x, int y);

public:
    void qmlEmitSignal1();
    void qmlEmitSignal2();
    void qmlEmitSignal2WithArgs(QString x, int y);
};

class ANON_javaScriptFunctions : public QObject, public ContextRegistrator
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int func1P READ getFunc1P WRITE setFunc1P)
    Q_PROPERTY(QString func2P READ getFunc2P WRITE setFunc2P)
    Q_PROPERTY(bool func3P READ getFunc3P WRITE setFunc3P)

public:
    // test workaround: the url is resolved by the test base class, so use
    // member variable to store the resolved url used as argument in engine
    // evaluation of runtime functions
    static QUrl url;

    ANON_javaScriptFunctions(QQmlEngine *e, QObject *parent = nullptr);

    int getFunc1P() { return func1P.value(); }
    QString getFunc2P() { return func2P.value(); }
    bool getFunc3P() { return func3P.value(); }

    void setFunc1P(const int &func1P_) { func1P.setValue(func1P_); }
    void setFunc2P(const QString &func2P_) { func2P.setValue(func2P_); }
    void setFunc3P(const bool &func3P_) { func3P.setValue(func3P_); }

    // try if just QProperty works
    QProperty<int> func1P;
    QProperty<QString> func2P;
    QProperty<bool> func3P;

    void func1();
    void func2(QString x);
    bool func3();
};

class ANON_changingBindings : public QObject, public ContextRegistrator
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int p1 READ getP1 WRITE setP1 BINDABLE bindableP1)
    Q_PROPERTY(int p2 READ getP2 WRITE setP2 BINDABLE bindableP2)

public:
    // test workaround: the url is resolved by the test base class, so use
    // member variable to store the resolved url used as argument in engine
    // evaluation of runtime functions
    static QUrl url;
    // test util to monitor binding execution
    int initialBindingCallCount = 0;
    // test util: allows to set C++ binding multiple times
    void resetToInitialBinding();

    ANON_changingBindings(QQmlEngine *e, QObject *parent = nullptr);

    int getP1() { return p1.value(); }
    int getP2() { return p2.value(); }

    void setP1(int p1_) { p1.setValue(p1_); }
    void setP2(int p2_) { p2.setValue(p2_); }

    QBindable<int> bindableP1() { return QBindable<int>(&p1); }
    QBindable<int> bindableP2() { return QBindable<int>(&p2); }

    Q_OBJECT_BINDABLE_PROPERTY(ANON_changingBindings, int, p1);
    Q_OBJECT_BINDABLE_PROPERTY(ANON_changingBindings, int, p2);

    void resetToConstant();
    void resetToNewBinding();
};

class ANON_propertyAlias : public QObject, public ContextRegistrator
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int dummy READ getDummy WRITE setDummy NOTIFY dummyChanged)
    Q_PROPERTY(int origin READ getOrigin WRITE setOrigin BINDABLE bindableOrigin)
    Q_PROPERTY(int aliasToOrigin READ getAliasToOrigin WRITE setAliasToOrigin BINDABLE
                       bindableAliasToOrigin)

public:
    // test workaround: the url is resolved by the test base class, so use
    // member variable to store the resolved url used as argument in engine
    // evaluation of runtime functions
    static QUrl url;
    // test util: allows to set C++ binding multiple times
    void resetToInitialBinding();

    ANON_propertyAlias(QQmlEngine *e, QObject *parent = nullptr);

    int getDummy() { return dummy.value(); }
    int getOrigin() { return origin.value(); }
    int getAliasToOrigin() { return getOrigin(); }

    void setDummy(int dummy_)
    {
        dummy.setValue(dummy_);
        // emit is essential for Qt.binding() to work correctly
        emit dummyChanged();
    }
    void setOrigin(int origin_) { origin.setValue(origin_); }
    void setAliasToOrigin(int aliasToOrigin_) { setOrigin(aliasToOrigin_); }

    QBindable<int> bindableOrigin() { return QBindable<int>(&origin); }
    QBindable<int> bindableAliasToOrigin() { return bindableOrigin(); }

    QProperty<int> dummy;
    QProperty<int> origin;

    void resetAliasToConstant();
    void resetOriginToConstant();
    void resetAliasToNewBinding();
    void resetOriginToNewBinding();
    int getAliasValue();

Q_SIGNALS:
    void dummyChanged();
};

class ANON_propertyChangeHandler : public QObject, public ContextRegistrator
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int dummy READ getDummy WRITE setDummy)
    Q_PROPERTY(int p READ getP WRITE setP BINDABLE bindableP)
    Q_PROPERTY(int watcher READ getWatcher WRITE setWatcher)

public:
    // test workaround: the url is resolved by the test base class, so use
    // member variable to store the resolved url used as argument in engine
    // evaluation of runtime functions
    static QUrl url;

    ANON_propertyChangeHandler(QQmlEngine *e, QObject *parent = nullptr);

    int getDummy() { return dummy.value(); }
    int getP() { return p.value(); }
    int getWatcher() { return watcher.value(); }

    void setDummy(int dummy_) { dummy.setValue(dummy_); }
    void setP(int p_) { p.setValue(p_); }
    void setWatcher(int watcher_) { watcher.setValue(watcher_); }

    QBindable<int> bindableP() { return QBindable<int>(&p); }

    QProperty<int> dummy;
    QProperty<int> p;
    QProperty<int> watcher;

    // property change handler:
    struct ANON_propertyChangeHandler_p_changeHandler
    {
        ANON_propertyChangeHandler *This = nullptr;
        ANON_propertyChangeHandler_p_changeHandler(ANON_propertyChangeHandler *obj);

        void operator()();
    };
    // the handler object has to be alive as long as the object
    std::unique_ptr<QPropertyChangeHandler<ANON_propertyChangeHandler_p_changeHandler>>
            pChangeHandler;
};

class LocallyImported : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int count READ getCount WRITE setCount BINDABLE bindableCount)

protected:
    LocallyImported(QObject *parent = nullptr);

public:
    // test workaround: the url is resolved by the test base class, so use
    // member variable to store the resolved url used as argument in engine
    // evaluation of runtime functions
    static QUrl url;

    LocallyImported(QQmlEngine *e, QObject *parent = nullptr);

    QQmlRefPointer<QQmlContextData> init(QQmlEngine *e,
                                         const QQmlRefPointer<QQmlContextData> &parentContext);
    void finalize(QQmlEngine *e);

    Q_INVOKABLE QVariant getMagicValue();
    void completedSlot();

    int getCount() { return count.value(); }
    void setCount(int count_) { count.setValue(count_); }
    QBindable<int> bindableCount() { return QBindable<int>(&count); }
    QProperty<int> count;
};

class ANON_localImport : public LocallyImported
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int p1 READ getP1 WRITE setP1)

protected:
    ANON_localImport(QObject *parent = nullptr);

public:
    // test workaround: the url is resolved by the test base class, so use
    // member variable to store the resolved url used as argument in engine
    // evaluation of runtime functions
    static QUrl url;

    ANON_localImport(QQmlEngine *e, QObject *parent = nullptr);

    QQmlRefPointer<QQmlContextData> init(QQmlEngine *e,
                                         const QQmlRefPointer<QQmlContextData> &parentContext);
    void finalize(QQmlEngine *e);

    int getP1() { return p1.value(); }
    void setP1(int p1_) { p1.setValue(p1_); }
    QProperty<int> p1;

    Q_INVOKABLE QVariant localGetMagicValue();
};

class ANON_neighbors_QtObject : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int p READ getP WRITE setP BINDABLE bindableP)
    Q_PROPERTY(int p2 READ getP2 WRITE setP2 BINDABLE bindableP2)

protected:
    ANON_neighbors_QtObject(QObject *parent = nullptr);

public:
    // test workaround: the url is resolved by the test base class, so use
    // member variable to store the resolved url used as argument in engine
    // evaluation of runtime functions
    static QUrl url;

    ANON_neighbors_QtObject(QQmlEngine *e, QObject *parent = nullptr);
    QQmlRefPointer<QQmlContextData> init(QQmlEngine *e,
                                         const QQmlRefPointer<QQmlContextData> &parentContext);
    void finalize(QQmlEngine *e); // called by the document root

    int getP() { return p.value(); }
    void setP(int p_) { p.setValue(p_); }
    QBindable<int> bindableP() { return QBindable<int>(&p); }
    QProperty<int> p;

    int getP2() { return p2.value(); }
    void setP2(int p2_) { p2.setValue(p2_); }
    QBindable<int> bindableP2() { return QBindable<int>(&p2); }
    QProperty<int> p2;
};

class ANON_neighbors_LocallyImported : public LocallyImported
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int p READ getP WRITE setP BINDABLE bindableP)

protected:
    ANON_neighbors_LocallyImported(QObject *parent = nullptr);

public:
    // test workaround: the url is resolved by the test base class, so use
    // member variable to store the resolved url used as argument in engine
    // evaluation of runtime functions
    static QUrl url2;

    ANON_neighbors_LocallyImported(QQmlEngine *e, QObject *parent = nullptr);

    QQmlRefPointer<QQmlContextData> init(QQmlEngine *e,
                                         const QQmlRefPointer<QQmlContextData> &parentContext);
    void finalize(QQmlEngine *e); // called by the document root

    int getP() { return p.value(); }
    void setP(int p_) { p.setValue(p_); }
    QBindable<int> bindableP() { return QBindable<int>(&p); }
    QProperty<int> p;
};

class ANON_neighbors : public QQuickItem
{
    Q_OBJECT
    QML_ANONYMOUS

protected:
    ANON_neighbors(QObject *parent = nullptr);

public:
    // test workaround: the url is resolved by the test base class, so use
    // member variable to store the resolved url used as argument in engine
    // evaluation of runtime functions
    static QUrl url;

    ANON_neighbors(QQmlEngine *e, QObject *parent = nullptr);
    QQmlRefPointer<QQmlContextData> init(QQmlEngine *e,
                                         const QQmlRefPointer<QQmlContextData> &parentContext);
    void finalize(QQmlEngine *e);
};

class ANON_anchors : public QQuickItem
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int value READ getValue WRITE setValue NOTIFY valueChanged)
protected:
    ANON_anchors(QObject *parent = nullptr);

public:
    // test workaround: the url is resolved by the test base class, so use
    // member variable to store the resolved url used as argument in engine
    // evaluation of runtime functions
    static QUrl url;

    ANON_anchors(QQmlEngine *e, QObject *parent = nullptr);
    QQmlRefPointer<QQmlContextData> init(QQmlEngine *e,
                                         const QQmlRefPointer<QQmlContextData> &parentContext);
    void finalize(QQmlEngine *e);

    QProperty<int> value;
    int getValue() { return value; }
    void setValue(int v)
    {
        value = v;
        Q_EMIT valueChanged();
    }
Q_SIGNALS:
    void valueChanged();
};

#endif // TESTCLASSES_H
