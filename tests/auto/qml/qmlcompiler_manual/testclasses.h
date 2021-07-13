/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

// utility class that sets up QQmlContext for passed QObject. can be used as a
// base class to ensure that qmlEngine(object) is valid during initializer list
// evaluation
struct ContextRegistrator
{
    ContextRegistrator(QQmlEngine *engine, QObject *This);
};

class HelloWorld : public QObject, public ContextRegistrator
{
    Q_OBJECT
    QML_NAMED_ELEMENT(HelloWorld);
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

class ANON_propertyReturningFunction : public QObject, public ContextRegistrator
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int counter READ getCounter WRITE setCounter)
    Q_PROPERTY(QVariant f READ getF WRITE setF BINDABLE bindableF)

public:
    // test workaround: the url is resolved by the test base class, so use
    // member variable to store the resolved url used as argument in engine
    // evaluation of runtime functions
    static QUrl url;

    ANON_propertyReturningFunction(QQmlEngine *e, QObject *parent = nullptr);

    int getCounter() { return counter.value(); }
    QVariant getF() { return f.value(); }

    void setCounter(int counter_) { counter.setValue(counter_); }
    void setF(QVariant f_) { f.setValue(f_); }

    QBindable<QVariant> bindableF() { return QBindable<QVariant>(&f); }

    QProperty<int> counter;
    QProperty<QVariant> f;
};

#endif // TESTCLASSES_H
