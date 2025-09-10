// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLENGINEDEBUGSERVICE_H
#define QQMLENGINEDEBUGSERVICE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qqmldebugservice_p.h>
#include <private/qqmldebugserviceinterfaces_p.h>

#include <QtCore/qurl.h>
#include <QtCore/qvariant.h>
#include <QtCore/QPointer>

QT_BEGIN_NAMESPACE

class QQmlEngine;
class QQmlContext;
class QQmlWatcher;
class QDataStream;
class QQmlDebugStatesDelegate;

class QQmlEngineDebugServiceImpl : public QQmlEngineDebugService
{
    Q_OBJECT
public:
    QQmlEngineDebugServiceImpl(QObject * = nullptr);
    ~QQmlEngineDebugServiceImpl();

    struct QQmlObjectData {
        QUrl url;
        int lineNumber;
        int columnNumber;
        QString idString;
        QString objectName;
        QString objectType;
        int objectId;
        int contextId;
        int parentId;
    };

    struct QQmlObjectProperty {
        enum Type { Unknown, Basic, Object, List, SignalProperty, Variant };
        Type type;
        QString name;
        QVariant value;
        QString valueTypeName;
        QString binding;
        bool hasNotifySignal;
    };

    void engineAboutToBeAdded(QJSEngine *) override;
    void engineAboutToBeRemoved(QJSEngine *) override;
    void objectCreated(QJSEngine *, QObject *) override;

    QQmlDebugStatesDelegate *statesDelegate()
    {
        if (!m_statesDelegate)
            m_statesDelegate = createStatesDelegate();
        return m_statesDelegate;
    }

Q_SIGNALS:
    void scheduleMessage(const QByteArray &);

protected:
    void messageReceived(const QByteArray &) override;

private:
    friend class QQmlDebuggerServiceFactory;

    void processMessage(const QByteArray &msg);
    void propertyChanged(qint32 id, qint32 objectId, const QMetaProperty &property,
                         const QVariant &value);

    void prepareDeferredObjects(QObject *);
    void buildObjectList(QDataStream &, QQmlContext *,
                         const QList<QPointer<QObject> > &instances);
    void buildObjectDump(QDataStream &, QObject *, bool, bool);
    void buildStatesList(bool cleanList, const QList<QPointer<QObject> > &instances);
    QQmlObjectData objectData(QObject *);
    QQmlObjectProperty propertyData(QObject *, int);
    QVariant valueContents(QVariant defaultValue) const;
    bool setBinding(int objectId, const QString &propertyName, const QVariant &expression, bool isLiteralValue, QString filename = QString(), int line = -1, int column = 0);
    bool resetBinding(int objectId, const QString &propertyName);
    bool setMethodBody(int objectId, const QString &method, const QString &body);
    void storeObjectIds(QObject *co);
    QList<QObject *> objectForLocationInfo(const QString &filename, int lineNumber,
                                           int columnNumber);

    QList<QJSEngine *> m_engines;
    QQmlWatcher *m_watch;
    QQmlDebugStatesDelegate *m_statesDelegate;
};
QDataStream &operator<<(QDataStream &, const QQmlEngineDebugServiceImpl::QQmlObjectData &);
QDataStream &operator>>(QDataStream &, QQmlEngineDebugServiceImpl::QQmlObjectData &);
QDataStream &operator<<(QDataStream &, const QQmlEngineDebugServiceImpl::QQmlObjectProperty &);
QDataStream &operator>>(QDataStream &, QQmlEngineDebugServiceImpl::QQmlObjectProperty &);

QT_END_NAMESPACE

#endif // QQMLENGINEDEBUGSERVICE_H

