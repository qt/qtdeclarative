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

#include "qqmlboundsignal_p.h"

#include <private/qmetaobjectbuilder_p.h>
#include "qqmlengine_p.h"
#include "qqmlexpression_p.h"
#include "qqmlcontext_p.h"
#include "qqmlmetatype_p.h"
#include "qqml.h"
#include "qqmlcontext.h"
#include "qqmlglobal_p.h"
#include <private/qqmlprofilerservice_p.h>
#include <private/qv8debugservice_p.h>

#include <QtCore/qstringbuilder.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

class QQmlBoundSignalParameters : public QObject
{
Q_OBJECT
public:
    QQmlBoundSignalParameters(const QMetaMethod &, QObject * = 0);
    ~QQmlBoundSignalParameters();

    void setValues(void **);
    void clearValues();

private:
    friend class MetaObject;
    int metaCall(QMetaObject::Call, int _id, void **);
    struct MetaObject : public QAbstractDynamicMetaObject {
        MetaObject(QQmlBoundSignalParameters *b)
            : parent(b) {}

        int metaCall(QMetaObject::Call c, int id, void **a) { 
            return parent->metaCall(c, id, a);
        }
        QQmlBoundSignalParameters *parent;
    };

    int *types;
    void **values;
    QMetaObject *myMetaObject;
};

static int evaluateIdx = -1;

QQmlAbstractBoundSignal::QQmlAbstractBoundSignal(QObject *parent)
: QObject(parent)
{
}

QQmlAbstractBoundSignal::~QQmlAbstractBoundSignal()
{
}

QQmlBoundSignal::QQmlBoundSignal(QObject *scope, const QMetaMethod &signal, 
                               QObject *parent)
: m_expression(0), m_signal(signal), m_paramsValid(false), m_isEvaluating(false), m_params(0)
{
    // This is thread safe.  Although it may be updated by two threads, they
    // will both set it to the same value - so the worst thing that can happen
    // is that they both do the work to figure it out.  Boo hoo.
    if (evaluateIdx == -1) evaluateIdx = metaObject()->methodCount();

    QQml_setParent_noEvent(this, parent);
    QQmlPropertyPrivate::connect(scope, m_signal.methodIndex(), this, evaluateIdx);
}

QQmlBoundSignal::QQmlBoundSignal(QQmlContext *ctxt, const QString &val, 
                               QObject *scope, const QMetaMethod &signal,
                               QObject *parent)
: m_expression(0), m_signal(signal), m_paramsValid(false), m_isEvaluating(false), m_params(0)
{
    // This is thread safe.  Although it may be updated by two threads, they
    // will both set it to the same value - so the worst thing that can happen
    // is that they both do the work to figure it out.  Boo hoo.
    if (evaluateIdx == -1) evaluateIdx = metaObject()->methodCount();

    QQml_setParent_noEvent(this, parent);
    QQmlPropertyPrivate::connect(scope, m_signal.methodIndex(), this, evaluateIdx);

    m_expression = new QQmlExpression(ctxt, scope, val);
}

QQmlBoundSignal::~QQmlBoundSignal()
{
    delete m_expression;
    m_expression = 0;
}

int QQmlBoundSignal::index() const 
{ 
    return m_signal.methodIndex();
}

/*!
    Returns the signal expression.
*/
QQmlExpression *QQmlBoundSignal::expression() const
{
    return m_expression;
}

/*!
    Sets the signal expression to \a e.  Returns the current signal expression,
    or null if there is no signal expression.

    The QQmlBoundSignal instance takes ownership of \a e.  The caller is 
    assumes ownership of the returned QQmlExpression.
*/
QQmlExpression *QQmlBoundSignal::setExpression(QQmlExpression *e)
{
    QQmlExpression *rv = m_expression;
    m_expression = e;
    if (m_expression) m_expression->setNotifyOnValueChanged(false);
    return rv;
}

QQmlBoundSignal *QQmlBoundSignal::cast(QObject *o)
{
    QQmlAbstractBoundSignal *s = qobject_cast<QQmlAbstractBoundSignal*>(o);
    return static_cast<QQmlBoundSignal *>(s);
}

int QQmlBoundSignal::qt_metacall(QMetaObject::Call c, int id, void **a)
{
    if (c == QMetaObject::InvokeMetaMethod && id == evaluateIdx) {
        if (!m_expression)
            return -1;

        if (QQmlDebugService::isDebuggingEnabled())
            QV8DebugService::instance()->signalEmitted(QString::fromAscii(m_signal.signature()));

        QQmlHandlingSignalProfiler prof(m_signal, m_expression);

        m_isEvaluating = true;
        if (!m_paramsValid) {
            if (!m_signal.parameterTypes().isEmpty())
                m_params = new QQmlBoundSignalParameters(m_signal, this);
            m_paramsValid = true;
        }

        if (m_params) m_params->setValues(a);
        if (m_expression && m_expression->engine()) {
            QQmlExpressionPrivate::get(m_expression)->value(m_params);
            if (m_expression && m_expression->hasError())
                QQmlEnginePrivate::warning(m_expression->engine(), m_expression->error());
        }
        if (m_params) m_params->clearValues();
        m_isEvaluating = false;
        return -1;
    } else {
        return QObject::qt_metacall(c, id, a);
    }
}

QQmlBoundSignalParameters::QQmlBoundSignalParameters(const QMetaMethod &method, 
                                                                     QObject *parent)
: QObject(parent), types(0), values(0)
{
    MetaObject *mo = new MetaObject(this);

    // ### Optimize!
    QMetaObjectBuilder mob;
    mob.setSuperClass(&QQmlBoundSignalParameters::staticMetaObject);
    mob.setClassName("QQmlBoundSignalParameters");

    QList<QByteArray> paramTypes = method.parameterTypes();
    QList<QByteArray> paramNames = method.parameterNames();
    types = new int[paramTypes.count()];
    for (int ii = 0; ii < paramTypes.count(); ++ii) {
        const QByteArray &type = paramTypes.at(ii);
        const QByteArray &name = paramNames.at(ii);

        if (name.isEmpty() || type.isEmpty()) {
            types[ii] = 0;
            continue;
        }

        QVariant::Type t = (QVariant::Type)QMetaType::type(type.constData());
        if (QQmlMetaType::isQObject(t)) {
            types[ii] = QMetaType::QObjectStar;
            QMetaPropertyBuilder prop = mob.addProperty(name, "QObject*");
            prop.setWritable(false);
        } else {
            QByteArray propType = type;
            if (t >= QVariant::UserType || t == QVariant::Invalid) {
                QByteArray scope;
                QByteArray name;
                int scopeIdx = propType.lastIndexOf("::");
                if (scopeIdx != -1) {
                    scope = propType.left(scopeIdx);
                    name = propType.mid(scopeIdx + 2);
                } else {
                    name = propType;
                }
                const QMetaObject *meta;
                if (scope == "Qt")
                    meta = &QObject::staticQtMetaObject;
                else
                    meta = parent->parent()->metaObject();   //### assumes parent->parent()
                for (int i = meta->enumeratorCount() - 1; i >= 0; --i) {
                    QMetaEnum m = meta->enumerator(i);
                    if ((m.name() == name) && (scope.isEmpty() || (m.scope() == scope))) {
                        t = QVariant::Int;
                        propType = "int";
                        break;
                    }
                }
            }
            types[ii] = t;
            QMetaPropertyBuilder prop = mob.addProperty(name, propType);
            prop.setWritable(false);
        }
    }
    myMetaObject = mob.toMetaObject();
    *static_cast<QMetaObject *>(mo) = *myMetaObject;

    d_ptr->metaObject = mo;
}

QQmlBoundSignalParameters::~QQmlBoundSignalParameters()
{
    delete [] types;
    free(myMetaObject);
}

void QQmlBoundSignalParameters::setValues(void **v)
{
    values = v;
}

void QQmlBoundSignalParameters::clearValues()
{
    values = 0;
}

int QQmlBoundSignalParameters::metaCall(QMetaObject::Call c, int id, void **a)
{
    if (!values)
        return -1;

    if (c == QMetaObject::ReadProperty && id >= 1) {
        int t = types[id - 1];
        void *p = a[0];
        QMetaType::destruct(t, p);
        QMetaType::construct(t, p, values[id]);
        return -1;
    } else {
        return qt_metacall(c, id, a);
    }
}

QT_END_NAMESPACE

#include <qqmlboundsignal.moc>
