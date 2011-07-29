/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QtDeclarative/qdeclarative.h>
#include <QtDeclarative/private/qdeclarativecustomparser_p.h>

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QVariant>
#include <QtQuick1/private/qlistmodelinterface_p.h>
#include <QtDeclarative/qjsvalue.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class FlatListModel_1;
class NestedListModel_1;
class QDeclarative1ListModelWorkerAgent;
struct ModelNode;
class FlatListScriptClass_1;
class Q_DECLARATIVE_PRIVATE_EXPORT QDeclarative1ListModel : public QListModelInterface
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    QDeclarative1ListModel(QObject *parent=0);
    ~QDeclarative1ListModel();

    virtual QList<int> roles() const;
    virtual QString toString(int role) const;
    virtual int count() const;
    virtual QVariant data(int index, int role) const;

    Q_INVOKABLE void clear();
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void append(const QScriptValue&);
    Q_INVOKABLE void insert(int index, const QScriptValue&);
    Q_INVOKABLE QScriptValue get(int index) const;
    Q_INVOKABLE void set(int index, const QScriptValue&);
    Q_INVOKABLE void setProperty(int index, const QString& property, const QVariant& value);
    Q_INVOKABLE void move(int from, int to, int count);
    Q_INVOKABLE void sync();

    QDeclarative1ListModelWorkerAgent *agent();

Q_SIGNALS:
    void countChanged();

private:
    friend class QDeclarative1ListModelParser;
    friend class QDeclarative1ListModelWorkerAgent;
    friend class FlatListModel_1;
    friend class FlatListScriptClass_1;
    friend struct ModelNode;

    // Constructs a flat list model for a worker agent
    QDeclarative1ListModel(const QDeclarative1ListModel *orig, QDeclarative1ListModelWorkerAgent *parent);

    void set(int index, const QScriptValue&, QList<int> *roles);
    void setProperty(int index, const QString& property, const QVariant& value, QList<int> *roles);

    bool flatten();
    bool inWorkerThread() const;

    inline bool canMove(int from, int to, int n) const { return !(from+n > count() || to+n > count() || from < 0 || to < 0 || n < 0); }

    QDeclarative1ListModelWorkerAgent *m_agent;
    NestedListModel_1 *m_nested;
    FlatListModel_1 *m_flat;
};

// ### FIXME
class QDeclarative1ListElement : public QObject
{
Q_OBJECT
};

class QDeclarative1ListModelParser : public QDeclarativeCustomParser
{
public:
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

    QByteArray listElementTypeName;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QDeclarative1ListModel)
QML_DECLARE_TYPE(QDeclarative1ListElement)

QT_END_HEADER

#endif // QDECLARATIVELISTMODEL_H
