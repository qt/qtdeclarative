/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVELISTMODEL_H
#define QDECLARATIVELISTMODEL_H

#include <qdeclarative.h>
#include <private/qdeclarativecustomparser_p.h>

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QVariant>
#include "qlistmodelinterface_p.h"

#include <private/qv8engine_p.h>
#include <private/qpodvector_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QDeclarativeListModelWorkerAgent;
class ListModel;
class ListLayout;

class Q_DECLARATIVE_PRIVATE_EXPORT QDeclarativeListModel : public QListModelInterface
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool dynamicRoles READ dynamicRoles WRITE setDynamicRoles)

public:
    QDeclarativeListModel(QObject *parent=0);
    ~QDeclarativeListModel();

    virtual QList<int> roles() const;
    virtual QString toString(int role) const;
    virtual int count() const;
    virtual QVariant data(int index, int role) const;

    Q_INVOKABLE void clear();
    Q_INVOKABLE void remove(QDeclarativeV8Function *args);
    Q_INVOKABLE void append(QDeclarativeV8Function *args);
    Q_INVOKABLE void insert(QDeclarativeV8Function *args);
    Q_INVOKABLE QDeclarativeV8Handle get(int index) const;
    Q_INVOKABLE void set(int index, const QDeclarativeV8Handle &);
    Q_INVOKABLE void setProperty(int index, const QString& property, const QVariant& value);
    Q_INVOKABLE void move(int from, int to, int count);
    Q_INVOKABLE void sync();

    QDeclarativeListModelWorkerAgent *agent();

    bool dynamicRoles() const { return m_dynamicRoles; }
    void setDynamicRoles(bool enableDynamicRoles);

Q_SIGNALS:
    void countChanged();

private:
    friend class QDeclarativeListModelParser;
    friend class QDeclarativeListModelWorkerAgent;
    friend class ModelObject;
    friend class ModelNodeMetaObject;
    friend class ListModel;
    friend class ListElement;
    friend class DynamicRoleModelNode;
    friend class DynamicRoleModelNodeMetaObject;

    // Constructs a flat list model for a worker agent
    QDeclarativeListModel(QDeclarativeListModel *orig, QDeclarativeListModelWorkerAgent *agent);
    QDeclarativeListModel(const QDeclarativeListModel *owner, ListModel *data, QV8Engine *eng, QObject *parent=0);

    QV8Engine *engine() const;

    inline bool canMove(int from, int to, int n) const { return !(from+n > count() || to+n > count() || from < 0 || to < 0 || n < 0); }

    QDeclarativeListModelWorkerAgent *m_agent;
    mutable QV8Engine *m_engine;
    bool m_mainThread;
    bool m_primary;

    bool m_dynamicRoles;

    ListLayout *m_layout;
    ListModel *m_listModel;

    QVector<class DynamicRoleModelNode *> m_modelObjects;
    QVector<QString> m_roles;
    int m_uid;

    struct ElementSync
    {
        ElementSync() : src(0), target(0) {}

        DynamicRoleModelNode *src;
        DynamicRoleModelNode *target;
    };

    int getUid() const { return m_uid; }

    static void sync(QDeclarativeListModel *src, QDeclarativeListModel *target, QHash<int, QDeclarativeListModel *> *targetModelHash);
    static QDeclarativeListModel *createWithOwner(QDeclarativeListModel *newOwner);

    void emitItemsChanged(int index, int count, const QList<int> &roles);
    void emitItemsRemoved(int index, int count);
    void emitItemsInserted(int index, int count);
    void emitItemsMoved(int from, int to, int n);
};

// ### FIXME
class QDeclarativeListElement : public QObject
{
Q_OBJECT
};

class QDeclarativeListModelParser : public QDeclarativeCustomParser
{
public:
    QDeclarativeListModelParser() : QDeclarativeCustomParser(QDeclarativeCustomParser::AcceptsSignalHandlers) {}
    QByteArray compile(const QList<QDeclarativeCustomParserProperty> &);
    void setCustomData(QObject *, const QByteArray &);

private:
    struct ListInstruction
    {
        enum { Push, Pop, Value, Set } type;
        int dataIdx;
    };
    struct ListModelData
    {
        int dataOffset;
        int instrCount;
        ListInstruction *instructions() const;
    };
    bool compileProperty(const QDeclarativeCustomParserProperty &prop, QList<ListInstruction> &instr, QByteArray &data);

    bool definesEmptyList(const QString &);

    QString listElementTypeName;

    struct DataStackElement
    {
        DataStackElement() : model(0), elementIndex(0) {}

        QString name;
        ListModel *model;
        int elementIndex;
    };
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarativeListModel)
QML_DECLARE_TYPE(QDeclarativeListElement)

QT_END_HEADER

#endif // QDECLARATIVELISTMODEL_H
