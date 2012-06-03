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

#include <private/qmetaobject_p.h>
#include <private/qmetaobjectbuilder_p.h>
#include "qqmlengine_p.h"
#include "qqmlexpression_p.h"
#include "qqmlcontext_p.h"
#include "qqmlmetatype_p.h"
#include "qqml.h"
#include "qqmlcontext.h"
#include "qqmlglobal_p.h"
#include "qqmlrewrite_p.h"
#include <private/qqmlprofilerservice_p.h>
#include <private/qv8debugservice_p.h>

#include <QtCore/qstringbuilder.h>
#include <QtCore/qdebug.h>

Q_DECLARE_METATYPE(QJSValue)

QT_BEGIN_NAMESPACE

static QQmlJavaScriptExpression::VTable QQmlBoundSignalExpression_jsvtable = {
    QQmlBoundSignalExpression::expressionIdentifier,
    QQmlBoundSignalExpression::expressionChanged
};

QQmlBoundSignalExpression::QQmlBoundSignalExpression(QQmlContextData *ctxt, QObject *scope, const QByteArray &expression,
                                                     bool isRewritten, const QString &fileName, int line, int column)
    : QQmlJavaScriptExpression(&QQmlBoundSignalExpression_jsvtable)
{
    setNotifyOnValueChanged(false);
    setContext(ctxt);
    setScopeObject(scope);
    if (isRewritten)
        m_expressionUtf8 = expression;
    else
        m_expression = QString::fromUtf8(expression);
    m_expressionFunctionValid = false;
    m_expressionFunctionRewritten = isRewritten;
    m_fileName = fileName;
    m_line = line;
    m_column = column;
}

QQmlBoundSignalExpression::QQmlBoundSignalExpression(QQmlContextData *ctxt, QObject *scope, const QString &expression,
                                                     bool isRewritten, const QString &fileName, int line, int column)
    : QQmlJavaScriptExpression(&QQmlBoundSignalExpression_jsvtable)
{
    setNotifyOnValueChanged(false);
    setContext(ctxt);
    setScopeObject(scope);
    if (isRewritten)
        m_expressionUtf8 = expression.toUtf8();
    else
        m_expression = expression;
    m_expressionFunctionValid = false;
    m_expressionFunctionRewritten = isRewritten;
    m_fileName = fileName;
    m_line = line;
    m_column = column;
}

QQmlBoundSignalExpression::~QQmlBoundSignalExpression()
{
    qPersistentDispose(m_v8function);
    qPersistentDispose(m_v8qmlscope);
}

QString QQmlBoundSignalExpression::expressionIdentifier(QQmlJavaScriptExpression *e)
{
    QQmlBoundSignalExpression *This = static_cast<QQmlBoundSignalExpression *>(e);
    return This->sourceFile() + QLatin1Char(':') + QString::number(This->lineNumber());
}

void QQmlBoundSignalExpression::expressionChanged(QQmlJavaScriptExpression *)
{
    // bound signals do not notify on change.
}

QString QQmlBoundSignalExpression::expression() const
{
    if (m_expressionFunctionValid) {
        Q_ASSERT (context() && engine());
        v8::HandleScope handle_scope;
        v8::Context::Scope context_scope(QQmlEnginePrivate::get(engine())->v8engine()->context());
        return QV8Engine::toStringStatic(m_v8function->ToString());
    } else if (!m_expressionUtf8.isEmpty()) {
        return QString::fromUtf8(m_expressionUtf8);
    } else {
        return m_expression;
    }
}

// This mirrors code in QQmlExpressionPrivate::value() and v8value().
// Any change made here should be made there and vice versa.
void QQmlBoundSignalExpression::evaluate(QObject *secondaryScope)
{
    Q_ASSERT (context() && engine());
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine());

    ep->referenceScarceResources(); // "hold" scarce resources in memory during evaluation.
    {
        v8::HandleScope handle_scope;
        v8::Context::Scope context_scope(ep->v8engine()->context());
        if (!m_expressionFunctionValid) {

            if (m_expressionFunctionRewritten) {
                m_v8function = evalFunction(context(), scopeObject(), QString::fromUtf8(m_expressionUtf8),
                                            m_fileName, m_line, &m_v8qmlscope);
                m_expressionUtf8.clear();
            } else {
                bool ok = true;
                QQmlRewrite::RewriteSignalHandler rewriteSignalHandler;
                const QString &code = rewriteSignalHandler(m_expression, QString()/*no name hint available*/, &ok);
                if (ok)
                    m_v8function = evalFunction(context(), scopeObject(), code, m_fileName, m_line, &m_v8qmlscope);
                m_expression.clear();
            }

            if (m_v8function.IsEmpty() || m_v8function->IsNull()) {
                ep->dereferenceScarceResources();
                return; // could not evaluate function.  Not valid.
            }

            setUseSharedContext(false);
            m_expressionFunctionValid = true;
        }

        if (secondaryScope) {
            QObject *restoreSecondaryScope = 0;
            restoreSecondaryScope = ep->v8engine()->contextWrapper()->setSecondaryScope(m_v8qmlscope, secondaryScope);
            QQmlJavaScriptExpression::evaluate(context(), m_v8function, 0);
            ep->v8engine()->contextWrapper()->setSecondaryScope(m_v8qmlscope, restoreSecondaryScope);
        } else {
            QQmlJavaScriptExpression::evaluate(context(), m_v8function, 0);
        }
    }
    ep->dereferenceScarceResources(); // "release" scarce resources if top-level expression evaluation is complete.
}

////////////////////////////////////////////////////////////////////////

class QQmlBoundSignalParameters : public QObject
{
Q_OBJECT
public:
    QQmlBoundSignalParameters(const QMetaMethod &, QQmlAbstractBoundSignal*);
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

QQmlAbstractBoundSignal::QQmlAbstractBoundSignal()
: m_prevSignal(0), m_nextSignal(0)
{
}

QQmlAbstractBoundSignal::~QQmlAbstractBoundSignal()
{
    removeFromObject();
}

void QQmlAbstractBoundSignal::addToObject(QObject *obj)
{
    Q_ASSERT(!m_prevSignal);
    Q_ASSERT(obj);

    QQmlData *data = QQmlData::get(obj, true);

    m_nextSignal = data->signalHandlers;
    if (m_nextSignal) m_nextSignal->m_prevSignal = &m_nextSignal;
    m_prevSignal = &data->signalHandlers;
    data->signalHandlers = this;
}

void QQmlAbstractBoundSignal::removeFromObject()
{
    if (m_prevSignal) {
        *m_prevSignal = m_nextSignal;
        if (m_nextSignal) m_nextSignal->m_prevSignal = m_prevSignal;
        m_prevSignal = 0;
        m_nextSignal = 0;
    }
}

/*! \internal
    \a signal MUST be in the signal index range (see QObjectPrivate::signalIndex()).
    This is different from QMetaMethod::methodIndex().
*/
QQmlBoundSignal::QQmlBoundSignal(QObject *scope, int signal, QObject *owner,
                                 QQmlEngine *engine)
: m_expression(0), m_params(0), m_scope(scope), m_index(signal)
{
    setParamsValid(false);
    setIsEvaluating(false);
    addToObject(owner);
    setCallback(QQmlNotifierEndpoint::QQmlBoundSignal);

    /*
        If this is a cloned method, connect to the 'original'. For example,
        for the signal 'void aSignal(int parameter = 0)', if the method
        index refers to 'aSignal()', get the index of 'aSignal(int)'.
        This ensures that 'parameter' will be available from QML.
    */
    if (QQmlData::get(scope, false) && QQmlData::get(scope, false)->propertyCache) {
        QQmlPropertyCache *cache = QQmlData::get(scope, false)->propertyCache;
        while (cache->signal(m_index)->isCloned())
            --m_index;
    } else {
        while (QMetaObjectPrivate::signal(scope->metaObject(), m_index).attributes() & QMetaMethod::Cloned)
            --m_index;
    }

    QQmlNotifierEndpoint::connect(scope, m_index, engine);
}

QQmlBoundSignal::~QQmlBoundSignal()
{
    m_expression = 0;
    delete m_params;
}

/*!
    Returns the signal index in the range returned by QObjectPrivate::signalIndex().
    This is different from QMetaMethod::methodIndex().
*/
int QQmlBoundSignal::index() const
{
    return m_index;
}

/*!
    Returns the signal expression.
*/
QQmlBoundSignalExpression *QQmlBoundSignal::expression() const
{
    return m_expression;
}

/*!
    Sets the signal expression to \a e.  Returns the current signal expression,
    or null if there is no signal expression.

    The QQmlBoundSignal instance adds a reference to \a e.  The caller
    assumes ownership of the returned QQmlBoundSignalExpression reference.
*/
QQmlBoundSignalExpressionPointer QQmlBoundSignal::setExpression(QQmlBoundSignalExpression *e)
{
    QQmlBoundSignalExpressionPointer rv = m_expression;
    m_expression = e;
    if (m_expression) m_expression->setNotifyOnValueChanged(false);
    return rv;
}

/*!
    Sets the signal expression to \a e.  Returns the current signal expression,
    or null if there is no signal expression.

    The QQmlBoundSignal instance takes ownership of \a e (and does not add a reference).  The caller
    assumes ownership of the returned QQmlBoundSignalExpression reference.
*/
QQmlBoundSignalExpressionPointer QQmlBoundSignal::takeExpression(QQmlBoundSignalExpression *e)
{
    QQmlBoundSignalExpressionPointer rv = m_expression;
    m_expression.take(e);
    if (m_expression) m_expression->setNotifyOnValueChanged(false);
    return rv;
}

void QQmlBoundSignal_callback(QQmlNotifierEndpoint *e, void **a)
{
    QQmlBoundSignal *s = static_cast<QQmlBoundSignal*>(e);
    if (!s->m_expression)
        return;

    if (QQmlDebugService::isDebuggingEnabled())
        QV8DebugService::instance()->signalEmitted(QString::fromLatin1(QMetaObjectPrivate::signal(s->m_scope->metaObject(), s->m_index).methodSignature()));

    QQmlHandlingSignalProfiler prof(s->m_expression);

    s->setIsEvaluating(true);

    if (!s->paramsValid()) {
        QList<QByteArray> names = QQmlPropertyCache::signalParameterNames(*s->m_scope, s->m_index);
        if (!names.isEmpty()) {
            QMetaMethod signal = QMetaObjectPrivate::signal(s->m_scope->metaObject(), s->m_index);
            s->m_params = new QQmlBoundSignalParameters(signal, s);
        }

        s->setParamsValid(true);
    }

    if (s->m_params) s->m_params->setValues(a);
    if (s->m_expression && s->m_expression->engine()) {
        s->m_expression->evaluate(s->m_params);
        if (s->m_expression && s->m_expression->hasError()) {
            QQmlEngine *engine = s->m_expression->engine();
            QQmlEnginePrivate::warning(engine, s->m_expression->error(engine));
        }
    }
    if (s->m_params) s->m_params->clearValues();

    s->setIsEvaluating(false);
}

QQmlBoundSignalParameters::QQmlBoundSignalParameters(const QMetaMethod &method, 
                                                     QQmlAbstractBoundSignal *owner)
: types(0), values(0)
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

        int t = QMetaType::type(type.constData());
        if (QQmlMetaType::isQObject(t)) {
            types[ii] = QMetaType::QObjectStar;
            QMetaPropertyBuilder prop = mob.addProperty(name, "QObject*");
            prop.setWritable(false);
        } else {
            QByteArray propType = type;
            QMetaType::TypeFlags flags = QMetaType::typeFlags(t);
            if (flags & QMetaType::IsEnumeration) {
                t = QVariant::Int;
                propType = "int";
            } else if (t == QMetaType::UnknownType ||
                       (t >= int(QMetaType::User) && !(flags & QMetaType::PointerToQObject) &&
                        t != qMetaTypeId<QJSValue>())) {
                //the UserType clause is to catch registered QFlags
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
                    meta = owner->scope()->metaObject();
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

////////////////////////////////////////////////////////////////////////

QQmlBoundSignalExpressionPointer::QQmlBoundSignalExpressionPointer(QQmlBoundSignalExpression *o)
: o(o)
{
    if (o) o->addref();
}

QQmlBoundSignalExpressionPointer::QQmlBoundSignalExpressionPointer(const QQmlBoundSignalExpressionPointer &other)
: o(other.o)
{
    if (o) o->addref();
}

QQmlBoundSignalExpressionPointer::~QQmlBoundSignalExpressionPointer()
{
    if (o) o->release();
}

QQmlBoundSignalExpressionPointer &QQmlBoundSignalExpressionPointer::operator=(const QQmlBoundSignalExpressionPointer &other)
{
    if (other.o) other.o->addref();
    if (o) o->release();
    o = other.o;
    return *this;
}

QQmlBoundSignalExpressionPointer &QQmlBoundSignalExpressionPointer::operator=(QQmlBoundSignalExpression *other)
{
    if (other) other->addref();
    if (o) o->release();
    o = other;
    return *this;
}

/*!
Takes ownership of \a other.  take() does *not* add a reference, as it assumes ownership
of the callers reference of other.
*/
QQmlBoundSignalExpressionPointer &QQmlBoundSignalExpressionPointer::take(QQmlBoundSignalExpression *other)
{
    if (o) o->release();
    o = other;
    return *this;
}

QT_END_NAMESPACE

#include <qqmlboundsignal.moc>
