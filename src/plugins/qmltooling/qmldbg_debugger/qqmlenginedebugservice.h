/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    QQmlEngineDebugServiceImpl(QObject * = 0);
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

    void engineAboutToBeAdded(QQmlEngine *) Q_DECL_OVERRIDE;
    void engineAboutToBeRemoved(QQmlEngine *) Q_DECL_OVERRIDE;
    void objectCreated(QQmlEngine *, QObject *) Q_DECL_OVERRIDE;

    void setStatesDelegate(QQmlDebugStatesDelegate *) Q_DECL_OVERRIDE;

protected:
    virtual void messageReceived(const QByteArray &) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void processMessage(const QByteArray &msg);
    void propertyChanged(int id, int objectId, const QMetaProperty &property, const QVariant &value);

private:
    friend class QQmlDebuggerServiceFactory;

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

    QList<QQmlEngine *> m_engines;
    QQmlWatcher *m_watch;
    QQmlDebugStatesDelegate *m_statesDelegate;
};
QDataStream &operator<<(QDataStream &, const QQmlEngineDebugServiceImpl::QQmlObjectData &);
QDataStream &operator>>(QDataStream &, QQmlEngineDebugServiceImpl::QQmlObjectData &);
QDataStream &operator<<(QDataStream &, const QQmlEngineDebugServiceImpl::QQmlObjectProperty &);
QDataStream &operator>>(QDataStream &, QQmlEngineDebugServiceImpl::QQmlObjectProperty &);

QT_END_NAMESPACE

#endif // QQMLENGINEDEBUGSERVICE_H

