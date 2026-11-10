// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef NAMEHOLDER_H
#define NAMEHOLDER_H

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qobject.h>
#include <QtCore/qttranslation.h>
#include <QtQml/qjsengine.h>
#include <QtQml/qjsvalue.h>
#include <QtQml/qqmlpropertymap.h>
#include <QtQml/qqmlregistration.h>

// We want methods named like the translation macros. Any file that includes this header can't
// use the macros anymore.
#undef QT_TR_NOOP
#undef QT_TRANSLATE_NOOP
#undef QT_TRID_NOOP

// Has a member for each of the names the tests look up. QML documents can't declare most of them,
// but C++ types can. Using any of them the way the tests do produces the objectName.
class NameHolder : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(NameHolder *console READ self CONSTANT FINAL)
    Q_PROPERTY(NameHolder *Math READ self CONSTANT FINAL)
    Q_PROPERTY(NameHolder *foo READ self CONSTANT FINAL)
    Q_PROPERTY(NameHolder *Plugh READ self CONSTANT FINAL)
    Q_PROPERTY(NameHolder *rect READ self CONSTANT FINAL)
    Q_PROPERTY(QString Infinity READ objectName CONSTANT FINAL)
    Q_PROPERTY(QString NaN READ objectName CONSTANT FINAL)
    Q_PROPERTY(QString undefined READ objectName CONSTANT FINAL)
    // The writable ones are only for the tests that assign to a name. escape is a member of the
    // global object, too.
    Q_PROPERTY(int stored MEMBER m_stored FINAL)
    Q_PROPERTY(int escape MEMBER m_escape FINAL)

public:
    explicit NameHolder(QObject *parent = nullptr) : QObject(parent) { }

    NameHolder *self() { return this; }
    Q_INVOKABLE QString debug(const QString &) const { return objectName(); }
    Q_INVOKABLE QString max(int, int) const { return objectName(); }

    Q_INVOKABLE QString qsTr(const QString &) const { return objectName(); }
    Q_INVOKABLE QString qsTranslate(const QString &, const QString &) const
    {
        return objectName();
    }
    Q_INVOKABLE QString qsTrId(const QString &) const { return objectName(); }
    Q_INVOKABLE QString QT_TR_NOOP(const QString &) const { return objectName(); }
    Q_INVOKABLE QString QT_TRANSLATE_NOOP(const QString &, const QString &) const
    {
        return objectName();
    }
    Q_INVOKABLE QString QT_TRID_NOOP(const QString &) const { return objectName(); }

private:
    int m_stored = 0;
    int m_escape = 0;
};

// Like NameHolder, but with the other kind of member for each name: The translation functions are
// properties holding JavaScript functions, and all the other names are methods. The methods return
// what NameHolder's properties hold. Using any of them produces the objectName, too, but the names
// that aren't translation functions have to be called first.
class SwappedNameHolder : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QJSValue qsTr READ translation CONSTANT FINAL)
    Q_PROPERTY(QJSValue qsTranslate READ translation CONSTANT FINAL)
    Q_PROPERTY(QJSValue qsTrId READ translation CONSTANT FINAL)
    Q_PROPERTY(QJSValue QT_TR_NOOP READ translation CONSTANT FINAL)
    Q_PROPERTY(QJSValue QT_TRANSLATE_NOOP READ translation CONSTANT FINAL)
    Q_PROPERTY(QJSValue QT_TRID_NOOP READ translation CONSTANT FINAL)

public:
    explicit SwappedNameHolder(QObject *parent = nullptr) : QObject(parent)
    {
        // The methods return the object itself. That must not hand it over to the JavaScript
        // garbage collector.
        QJSEngine::setObjectOwnership(this, QJSEngine::CppOwnership);
    }

    QJSValue translation() const
    {
        QJSEngine *engine = qjsEngine(this);
        Q_ASSERT(engine);
        return engine
                ->evaluate(QStringLiteral("(function(name) { return function() { return name } })"))
                .call({ objectName() });
    }

    Q_INVOKABLE SwappedNameHolder *console() { return this; }
    Q_INVOKABLE SwappedNameHolder *Math() { return this; }
    Q_INVOKABLE SwappedNameHolder *foo() { return this; }
    Q_INVOKABLE SwappedNameHolder *Plugh() { return this; }
    Q_INVOKABLE SwappedNameHolder *rect() { return this; }
    Q_INVOKABLE QString Infinity() const { return objectName(); }
    Q_INVOKABLE QString NaN() const { return objectName(); }
    Q_INVOKABLE QString undefined() const { return objectName(); }

    Q_INVOKABLE QString debug(const QString &) const { return objectName(); }
    Q_INVOKABLE QString max(int, int) const { return objectName(); }
};

// Has a dynamic metaobject. Names can be inserted at any time. QML documents cannot declare
// properties or functions on it, but they can bind its C++ properties. Changing the trigger
// re-evaluates a binding that reads it.
class DynamicNameHolder : public QQmlPropertyMap
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString probe MEMBER m_probe NOTIFY probeChanged FINAL)
    Q_PROPERTY(int trigger MEMBER m_trigger NOTIFY triggerChanged FINAL)

public:
    explicit DynamicNameHolder(QObject *parent = nullptr) : QQmlPropertyMap(this, parent) { }

Q_SIGNALS:
    void probeChanged();
    void triggerChanged();

private:
    QString m_probe;
    int m_trigger = 0;
};

// A single row with a role for each name. Each role has the same value.
class NameModel : public QAbstractListModel
{
    Q_OBJECT

public:
    NameModel(const QList<QByteArray> &names, const QVariant &value, QObject *parent = nullptr)
        : QAbstractListModel(parent), m_value(value)
    {
        for (qsizetype i = 0; i < names.size(); ++i)
            m_roleNames.insert(Qt::UserRole + int(i), names[i]);
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 1;
    }

    QVariant data(const QModelIndex &, int role) const override
    {
        return m_roleNames.contains(role) ? m_value : QVariant();
    }

    QHash<int, QByteArray> roleNames() const override { return m_roleNames; }

private:
    QHash<int, QByteArray> m_roleNames;
    QVariant m_value;
};

#endif // NAMEHOLDER_H
