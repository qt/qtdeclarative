// Commit: ac5c099cc3c5b8c7eec7a49fdeb8a21037230350
/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSGVISUALITEMMODEL_P_H
#define QSGVISUALITEMMODEL_P_H

#include <QtDeclarative/qdeclarative.h>
#include <QtDeclarative/qsgitem.h>
#include <QtCore/qobject.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtScript/qscriptvalue.h>

#include <private/qdeclarativeguard_p.h>
#include <private/qintrusivelist_p.h>

QT_BEGIN_HEADER

Q_DECLARE_METATYPE(QModelIndex)

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGItem;
class QDeclarativeComponent;
class QDeclarativePackage;
class QSGVisualDataModelPrivate;

class QSGVisualData;
class QSGVisualModelAttached;
class QSGVisualModelRole;
class QSGVisualModelPrivate;
class Q_DECLARATIVE_EXPORT QSGVisualModel : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSGVisualModel)

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QDeclarativeListProperty<QObject> data READ data DESIGNABLE false)
    Q_PROPERTY(QDeclarativeListProperty<QSGItem> children READ children NOTIFY childrenChanged DESIGNABLE false)
    Q_PROPERTY(QDeclarativeListProperty<QSGVisualModelRole> roles READ roles CONSTANT)
    Q_PROPERTY(QObject *parts READ parts CONSTANT)

    Q_CLASSINFO("DefaultProperty", "data")

public:
    enum ReleaseFlag { Referenced = 0x01, Destroyed = 0x02 };
    Q_DECLARE_FLAGS(ReleaseFlags, ReleaseFlag)

    QSGVisualModel(QObject *parent=0);
    virtual ~QSGVisualModel() {}

    int count() const;
    QObject *object(int index, bool complete = true);
    ReleaseFlags release(QObject *item);
    bool completePending() const;
    void completeItem();
    QString stringValue(int index, const QString &role);
    void setWatchedRoles(QList<QByteArray>) {}

    virtual int indexOf(QObject *item, QObject *objectContext) const;

    Q_INVOKABLE QScriptValue getItemInfo(int index) const;

    QDeclarativeListProperty<QObject> data();
    QDeclarativeListProperty<QSGItem> children();
    QDeclarativeListProperty<QSGVisualModelRole> roles();

    QObject *parts();

    static QSGVisualModelAttached *qmlAttachedProperties(QObject *obj);

    Q_INVOKABLE QSGItem *item(int index, bool complete = true);
    Q_INVOKABLE QSGItem *item(int index, const QByteArray &viewId, bool complete = true);
    Q_INVOKABLE QSGItem *take(int index, QSGItem *parent = 0);

public Q_SLOTS:
    void append(QSGItem *item);
    void append(QSGVisualModel *sourceModel, int sourceIndex, int count);
    void append(QDeclarativeComponent *delegate, const QVariant &model, int sourceIndex, int count);
    void append(QDeclarativeComponent *delegate, const QScriptValue &data);
    void insert(int index, QSGItem *item);
    void insert(int destinationIndex, QSGVisualModel *sourceModel, int sourceIndex, int count);
    void insert(int destinationIndex, QDeclarativeComponent *delegate, const QVariant &model, int sourceIndex, int count);
    void insert(int index, QDeclarativeComponent *delegate, const QScriptValue &data);
    void remove(int index, int count);
    void move(int from, int to, int count);

Q_SIGNALS:
    void childrenChanged();
    void countChanged();
    void itemsInserted(int index, int count);
    void itemsRemoved(int index, int count);
    void itemsMoved(int from, int to, int count);
    void itemsChanged(int index, int count);
    void modelReset();
    void createdItem(int index, QSGItem *item);
    void destroyingItem(QSGItem *item);

    void createdPackage(int index, QDeclarativePackage *package);
    void destroyingPackage(QDeclarativePackage *package);

    void updated(const QScriptValue &inserts);

protected:
    QSGVisualModel(QSGVisualModelPrivate &dd, QObject *parent);

private Q_SLOTS:
    void _q_itemsInserted(QSGVisualData *model, int index, int count);
    void _q_itemsRemoved(QSGVisualData *model, int index, int count);
    void _q_itemsMoved(QSGVisualData *model, int from, int to, int count);
    void _q_itemsChanged(QSGVisualData *model, int index, int count);

private:
    Q_DISABLE_COPY(QSGVisualModel)
};

class QSGVisualDataPrivate;
class Q_DECLARATIVE_EXPORT QSGVisualData : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSGVisualData)

    Q_PROPERTY(QVariant model READ model WRITE setModel)
    Q_PROPERTY(QDeclarativeComponent *delegate READ delegate WRITE setDelegate)
    Q_PROPERTY(QVariant rootIndex READ rootIndex WRITE setRootIndex NOTIFY rootIndexChanged)
    Q_CLASSINFO("DefaultProperty", "delegate")
public:
    QSGVisualData(QObject *parent = 0);
    QSGVisualData(QDeclarativeContext *, QObject *parent=0);
    virtual ~QSGVisualData();

    QVariant model() const;
    void setModel(const QVariant &);

    QDeclarativeComponent *delegate() const;
    void setDelegate(QDeclarativeComponent *);

    QVariant rootIndex() const;
    void setRootIndex(const QVariant &root);

    Q_INVOKABLE QVariant modelIndex(int idx) const;
    Q_INVOKABLE QVariant parentModelIndex() const;

    int count() const;
    QObject *object(int index, bool complete=true);
    void release(QObject *object);
    bool completePending() const;
    void completeItem();
    QString stringValue(int index, QObject *object, const QString &role);
    void setWatchedRoles(QList<QByteArray> roles);

    int indexOf(QObject *item, QObject *objectContext) const;

    bool updateData(int idx, QObject *object);

Q_SIGNALS:
    void itemsInserted(QSGVisualData *data, int index, int count);
    void itemsRemoved(QSGVisualData *data, int index, int count);
    void itemsMoved(QSGVisualData *data, int from, int to, int count);
    void itemsChanged(QSGVisualData *data, int index, int count);

    void rootIndexChanged();

private Q_SLOTS:
    void _q_itemsChanged(int, int, const QList<int> &);
    void _q_itemsInserted(int index, int count);
    void _q_itemsRemoved(int index, int count);
    void _q_itemsMoved(int from, int to, int count);
    void _q_rowsInserted(const QModelIndex &,int,int);
    void _q_rowsRemoved(const QModelIndex &,int,int);
    void _q_rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int);
    void _q_dataChanged(const QModelIndex&,const QModelIndex&);
    void _q_layoutChanged();
    void _q_modelReset();

private:
    Q_DISABLE_COPY(QSGVisualData)
};

class QSGVisualModelRole : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QVariant defaultValue READ defaultValue WRITE setDefaultValue NOTIFY defaultValueChanged)
public:
    QSGVisualModelRole(QObject *parent = 0) : QObject(parent) {}
    ~QSGVisualModelRole() {}

    QString name() const { return m_name; }
    void setName(const QString &name)
    {
        if (m_name == name)
            return;
        m_name = name;
        emit nameChanged();
    }

    QVariant defaultValue() const { return m_defaultValue; }
    void setDefaultValue(const QVariant &value)
    {
        if (m_defaultValue == value)
            return;
        m_defaultValue = value;
        emit defaultValueChanged();
    }

Q_SIGNALS:
    void nameChanged();
    void defaultValueChanged();

private:
    QString m_name;
    QVariant m_defaultValue;
};

class QSGVisualModelAttached : public QObject
{
    Q_OBJECT

public:
    QSGVisualModelAttached(QObject *parent)
        : QObject(parent), m_index(-1) {}
    ~QSGVisualModelAttached() {
        attachedProperties.remove(parent());
    }

    Q_PROPERTY(int index READ index NOTIFY indexChanged)
    int index() {
        if (m_index == -1 && m_model)
            m_index = m_model->indexOf(qobject_cast<QSGItem *>(parent()), 0);
        return m_index;
    }
    void setIndex(int idx) {
        if (m_index != idx) {
            m_index = idx;
            emit indexChanged();
        }
    }

    void setModel(QSGVisualModel *model) { m_model = model; }
    void setData(QSGVisualData *data) { m_data = data; }

    static QSGVisualModelAttached *properties(QObject *obj) {
        QSGVisualModelAttached *rv = attachedProperties.value(obj);
        if (!rv) {
            rv = new QSGVisualModelAttached(obj);
            attachedProperties.insert(obj, rv);
        }
        return rv;
    }

Q_SIGNALS:
    void indexChanged();

public:
    int m_index;
    QDeclarativeGuard<QSGVisualModel> m_model;
    QDeclarativeGuard<QSGVisualData> m_data;
    QIntrusiveListNode m_node;

    typedef QIntrusiveList<QSGVisualModelAttached, &QSGVisualModelAttached::m_node> List;

    static QHash<QObject*, QSGVisualModelAttached*> attachedProperties;
};

class Q_DECLARATIVE_EXPORT QSGVisualItemModel : public QSGVisualModel
{
    Q_OBJECT
    Q_CLASSINFO("DefaultProperty", "children")
public:
    QSGVisualItemModel(QObject *parent = 0);
    ~QSGVisualItemModel();

private:
    Q_DISABLE_COPY(QSGVisualItemModel)
};

class QSGVisualDataModelPrivate;
class Q_DECLARATIVE_EXPORT QSGVisualDataModel : public QSGVisualModel
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSGVisualDataModel)

    Q_PROPERTY(QVariant model READ model WRITE setModel)
    Q_PROPERTY(QDeclarativeComponent *delegate READ delegate WRITE setDelegate)
    Q_PROPERTY(QVariant rootIndex READ rootIndex WRITE setRootIndex NOTIFY rootIndexChanged)
    Q_CLASSINFO("DefaultProperty", "delegate")
public:
    QSGVisualDataModel();
    QSGVisualDataModel(QDeclarativeContext *, QObject *parent=0);
    virtual ~QSGVisualDataModel();

    QVariant model() const;
    void setModel(const QVariant &);

    QDeclarativeComponent *delegate() const;
    void setDelegate(QDeclarativeComponent *);

    QVariant rootIndex() const;
    void setRootIndex(const QVariant &root);

    Q_INVOKABLE QVariant modelIndex(int idx) const;
    Q_INVOKABLE QVariant parentModelIndex() const;

public Q_SLOTS:
    void appendData(const QScriptValue &value);
    void insertData(int index, const QScriptValue &value);

Q_SIGNALS:
    void rootIndexChanged();

private:
    Q_DISABLE_COPY(QSGVisualDataModel)
};

class Q_DECLARATIVE_EXPORT QSGVisualPartModel : public QObject
{
    Q_OBJECT
public:
    QSGVisualPartModel(QSGVisualModel *model, const QByteArray &part, QObject *parent = 0);

    QSGVisualModel *model() const;
    QByteArray part() const;

    Q_INVOKABLE QSGItem *item(int index);
    Q_INVOKABLE QSGItem *take(int index, QSGItem *parent = 0);

private:
    QSGVisualModel *m_model;
    QByteArray m_part;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QSGVisualModel)
QML_DECLARE_TYPEINFO(QSGVisualModel, QML_HAS_ATTACHED_PROPERTIES)
QML_DECLARE_TYPE(QSGVisualData)
QML_DECLARE_TYPE(QSGVisualModelRole)
QML_DECLARE_TYPEINFO(QSGVisualDataModel, QML_HAS_ATTACHED_PROPERTIES)
QML_DECLARE_TYPE(QSGVisualItemModel)
QML_DECLARE_TYPEINFO(QSGVisualItemModel, QML_HAS_ATTACHED_PROPERTIES)
QML_DECLARE_TYPE(QSGVisualDataModel)

QT_END_HEADER

#endif // QSGVISUALITEMMODEL_P_H
