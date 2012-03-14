/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLENGINEDEBUG_H
#define QQMLENGINEDEBUG_H

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

#include <QtCore/qobject.h>
#include <QtCore/qurl.h>
#include <QtCore/qvariant.h>

#include <private/qtqmlglobal_p.h>

class QQmlDebugConnection;
class QQmlDebugWatch;
class QQmlDebugPropertyWatch;
class QQmlDebugObjectExpressionWatch;
class QQmlDebugEnginesQuery;
class QQmlDebugRootContextQuery;
class QQmlDebugObjectQuery;
class QQmlDebugExpressionQuery;
class QQmlDebugPropertyReference;
class QQmlDebugContextReference;
class QQmlDebugObjectReference;
class QQmlDebugFileReference;
class QQmlDebugEngineReference;
class QQmlEngineDebugPrivate;
class QQmlEngineDebug : public QObject
{
    Q_OBJECT
public:
    enum State { NotConnected, Unavailable, Enabled };

    explicit QQmlEngineDebug(QQmlDebugConnection *, QObject * = 0);
    ~QQmlEngineDebug();

    State state() const;

    QQmlDebugPropertyWatch *addWatch(const QQmlDebugPropertyReference &,
                                             QObject *parent = 0);
    QQmlDebugWatch *addWatch(const QQmlDebugContextReference &, const QString &,
                                     QObject *parent = 0);
    QQmlDebugObjectExpressionWatch *addWatch(const QQmlDebugObjectReference &, const QString &,
                                                     QObject *parent = 0);
    QQmlDebugWatch *addWatch(const QQmlDebugObjectReference &,
                                     QObject *parent = 0);
    QQmlDebugWatch *addWatch(const QQmlDebugFileReference &,
                                     QObject *parent = 0);

    void removeWatch(QQmlDebugWatch *watch);

    QQmlDebugEnginesQuery *queryAvailableEngines(QObject *parent = 0);
    QQmlDebugRootContextQuery *queryRootContexts(const QQmlDebugEngineReference &,
                                                         QObject *parent = 0);
    QQmlDebugObjectQuery *queryObject(const QQmlDebugObjectReference &,
                                              QObject *parent = 0);
    QQmlDebugObjectQuery *queryObjectRecursive(const QQmlDebugObjectReference &,
                                                       QObject *parent = 0);
    QQmlDebugExpressionQuery *queryExpressionResult(int objectDebugId,
                                                            const QString &expr,
                                                            QObject *parent = 0);
    bool setBindingForObject(int objectDebugId, const QString &propertyName,
                             const QVariant &bindingExpression, bool isLiteralValue,
                             QString source = QString(), int line = -1);
    bool resetBindingForObject(int objectDebugId, const QString &propertyName);
    bool setMethodBody(int objectDebugId, const QString &methodName, const QString &methodBody);

Q_SIGNALS:
    void newObjects();
    void stateChanged(State state);

private:
    Q_DECLARE_PRIVATE(QQmlEngineDebug)
};

class QQmlDebugWatch : public QObject
{
    Q_OBJECT
public:
    enum State { Waiting, Active, Inactive, Dead };

    QQmlDebugWatch(QObject *);
    ~QQmlDebugWatch();

    int queryId() const;
    int objectDebugId() const;
    State state() const;

Q_SIGNALS:
    void stateChanged(QQmlDebugWatch::State);
    //void objectChanged(int, const QQmlDebugObjectReference &);
    //void valueChanged(int, const QVariant &);

    // Server sends value as string if it is a user-type variant
    void valueChanged(const QByteArray &name, const QVariant &value);

private:
    friend class QQmlEngineDebug;
    friend class QQmlEngineDebugPrivate;
    void setState(State);
    State m_state;
    int m_queryId;
    QQmlEngineDebug *m_client;
    int m_objectDebugId;
};

class QQmlDebugPropertyWatch : public QQmlDebugWatch
{
    Q_OBJECT
public:
    QQmlDebugPropertyWatch(QObject *parent);

    QString name() const;

private:
    friend class QQmlEngineDebug;
    QString m_name;
};

class QQmlDebugObjectExpressionWatch : public QQmlDebugWatch
{
    Q_OBJECT
public:
    QQmlDebugObjectExpressionWatch(QObject *parent);

    QString expression() const;

private:
    friend class QQmlEngineDebug;
    QString m_expr;
    int m_debugId;
};


class QQmlDebugQuery : public QObject
{
    Q_OBJECT
public:
    enum State { Waiting, Error, Completed };

    State state() const;
    bool isWaiting() const;

Q_SIGNALS:
    void stateChanged(QQmlDebugQuery::State);

protected:
    QQmlDebugQuery(QObject *);

private:
    friend class QQmlEngineDebug;
    friend class QQmlEngineDebugPrivate;
    void setState(State);
    State m_state;
};

class QQmlDebugFileReference
{
public:
    QQmlDebugFileReference();
    QQmlDebugFileReference(const QQmlDebugFileReference &);
    QQmlDebugFileReference &operator=(const QQmlDebugFileReference &);

    QUrl url() const;
    void setUrl(const QUrl &);
    int lineNumber() const;
    void setLineNumber(int);
    int columnNumber() const;
    void setColumnNumber(int);

private:
    friend class QQmlEngineDebugPrivate;
    QUrl m_url;
    int m_lineNumber;
    int m_columnNumber;
};

class QQmlDebugEngineReference
{
public:
    QQmlDebugEngineReference();
    QQmlDebugEngineReference(int);
    QQmlDebugEngineReference(const QQmlDebugEngineReference &);
    QQmlDebugEngineReference &operator=(const QQmlDebugEngineReference &);

    int debugId() const;
    QString name() const;

private:
    friend class QQmlEngineDebugPrivate;
    int m_debugId;
    QString m_name;
};

class QQmlDebugObjectReference
{
public:
    QQmlDebugObjectReference();
    QQmlDebugObjectReference(int);
    QQmlDebugObjectReference(const QQmlDebugObjectReference &);
    QQmlDebugObjectReference &operator=(const QQmlDebugObjectReference &);

    int debugId() const;
    QString className() const;
    QString idString() const;
    QString name() const;

    QQmlDebugFileReference source() const;
    int contextDebugId() const;

    QList<QQmlDebugPropertyReference> properties() const;
    QList<QQmlDebugObjectReference> children() const;

private:
    friend class QQmlEngineDebugPrivate;
    int m_debugId;
    QString m_class;
    QString m_idString;
    QString m_name;
    QQmlDebugFileReference m_source;
    int m_contextDebugId;
    QList<QQmlDebugPropertyReference> m_properties;
    QList<QQmlDebugObjectReference> m_children;
};

class QQmlDebugContextReference
{
public:
    QQmlDebugContextReference();
    QQmlDebugContextReference(const QQmlDebugContextReference &);
    QQmlDebugContextReference &operator=(const QQmlDebugContextReference &);

    int debugId() const;
    QString name() const;

    QList<QQmlDebugObjectReference> objects() const;
    QList<QQmlDebugContextReference> contexts() const;

private:
    friend class QQmlEngineDebugPrivate;
    int m_debugId;
    QString m_name;
    QList<QQmlDebugObjectReference> m_objects;
    QList<QQmlDebugContextReference> m_contexts;
};

class QQmlDebugPropertyReference
{
public:
    QQmlDebugPropertyReference();
    QQmlDebugPropertyReference(const QQmlDebugPropertyReference &);
    QQmlDebugPropertyReference &operator=(const QQmlDebugPropertyReference &);

    int objectDebugId() const;
    QString name() const;
    QVariant value() const;
    QString valueTypeName() const;
    QString binding() const;
    bool hasNotifySignal() const;

private:
    friend class QQmlEngineDebugPrivate;
    int m_objectDebugId;
    QString m_name;
    QVariant m_value;
    QString m_valueTypeName;
    QString m_binding;
    bool m_hasNotifySignal;
};


class QQmlDebugEnginesQuery : public QQmlDebugQuery
{
    Q_OBJECT
public:
    virtual ~QQmlDebugEnginesQuery();
    QList<QQmlDebugEngineReference> engines() const;
private:
    friend class QQmlEngineDebug;
    friend class QQmlEngineDebugPrivate;
    QQmlDebugEnginesQuery(QObject *);
    QQmlEngineDebug *m_client;
    int m_queryId;
    QList<QQmlDebugEngineReference> m_engines;
};

class QQmlDebugRootContextQuery : public QQmlDebugQuery
{
    Q_OBJECT
public:
    virtual ~QQmlDebugRootContextQuery();
    QQmlDebugContextReference rootContext() const;
private:
    friend class QQmlEngineDebug;
    friend class QQmlEngineDebugPrivate;
    QQmlDebugRootContextQuery(QObject *);
    QQmlEngineDebug *m_client;
    int m_queryId;
    QQmlDebugContextReference m_context;
};

class QQmlDebugObjectQuery : public QQmlDebugQuery
{
    Q_OBJECT
public:
    virtual ~QQmlDebugObjectQuery();
    QQmlDebugObjectReference object() const;
private:
    friend class QQmlEngineDebug;
    friend class QQmlEngineDebugPrivate;
    QQmlDebugObjectQuery(QObject *);
    QQmlEngineDebug *m_client;
    int m_queryId;
    QQmlDebugObjectReference m_object;

};

class QQmlDebugExpressionQuery : public QQmlDebugQuery
{
    Q_OBJECT
public:
    virtual ~QQmlDebugExpressionQuery();
    QVariant expression() const;
    QVariant result() const;
private:
    friend class QQmlEngineDebug;
    friend class QQmlEngineDebugPrivate;
    QQmlDebugExpressionQuery(QObject *);
    QQmlEngineDebug *m_client;
    int m_queryId;
    QVariant m_expr;
    QVariant m_result;
};

Q_DECLARE_METATYPE(QQmlDebugEngineReference)
Q_DECLARE_METATYPE(QQmlDebugObjectReference)
Q_DECLARE_METATYPE(QQmlDebugContextReference)
Q_DECLARE_METATYPE(QQmlDebugPropertyReference)

#endif // QQMLENGINEDEBUG_H
