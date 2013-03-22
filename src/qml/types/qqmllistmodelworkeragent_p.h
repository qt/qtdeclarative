/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKLISTMODELWORKERAGENT_P_H
#define QQUICKLISTMODELWORKERAGENT_P_H

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

#include <qqml.h>

#include <QMutex>
#include <QWaitCondition>

#include <private/qv8engine_p.h>

QT_BEGIN_NAMESPACE


class QQmlListModel;

class QQmlListModelWorkerAgent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int count READ count)

public:
    QQmlListModelWorkerAgent(QQmlListModel *);
    ~QQmlListModelWorkerAgent();
    void setV8Engine(QV8Engine *eng);

    void addref();
    void release();

    int count() const;

    Q_INVOKABLE void clear();
    Q_INVOKABLE void remove(QQmlV8Function *args);
    Q_INVOKABLE void append(QQmlV8Function *args);
    Q_INVOKABLE void insert(QQmlV8Function *args);
    Q_INVOKABLE QQmlV8Handle get(int index) const;
    Q_INVOKABLE void set(int index, const QQmlV8Handle &);
    Q_INVOKABLE void setProperty(int index, const QString& property, const QVariant& value);
    Q_INVOKABLE void move(int from, int to, int count);
    Q_INVOKABLE void sync();

    struct VariantRef
    {
        VariantRef() : a(0) {}
        VariantRef(const VariantRef &r) : a(r.a) { if (a) a->addref(); }
        VariantRef(QQmlListModelWorkerAgent *_a) : a(_a) { if (a) a->addref(); }
        ~VariantRef() { if (a) a->release(); }

        VariantRef &operator=(const VariantRef &o) { 
            if (o.a) o.a->addref(); 
            if (a) a->release(); a = o.a; 
            return *this; 
        }

        QQmlListModelWorkerAgent *a;
    };
    void modelDestroyed();
protected:
    virtual bool event(QEvent *);

private:
    friend class QQuickWorkerScriptEnginePrivate;
    friend class QQmlListModel;

    struct Change
    {
        int modelUid;
        enum { Inserted, Removed, Moved, Changed } type;
        int index; // Inserted/Removed/Moved/Changed
        int count; // Inserted/Removed/Moved/Changed
        int to;    // Moved
        QVector<int> roles;
    };

    struct Data
    {
        QList<Change> changes;

        void clearChange(int uid);
        void insertChange(int uid, int index, int count);
        void removeChange(int uid, int index, int count);
        void moveChange(int uid, int index, int count, int to);
        void changedChange(int uid, int index, int count, const QVector<int> &roles);
    };
    Data data;

    struct Sync : public QEvent {
        Sync() : QEvent(QEvent::User) {}
        Data data;
        QQmlListModel *list;
    };

    QAtomicInt m_ref;
    QQmlListModel *m_orig;
    QQmlListModel *m_copy;
    QMutex mutex;
    QWaitCondition syncDone;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQmlListModelWorkerAgent::VariantRef)

#endif // QQUICKLISTMODELWORKERAGENT_P_H

