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

#include "qv8engine_p.h"

#include "qv4sequenceobject_p.h"
#include "private/qjsengine_p.h"

#include <private/qqmlbuiltinfunctions_p.h>
#include <private/qqmllist_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlxmlhttprequest_p.h>
#include <private/qqmllocale_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qqmlmemoryprofiler_p.h>
#include <private/qqmlplatform_p.h>
#include <private/qjsvalue_p.h>
#include <private/qqmltypewrapper_p.h>
#include <private/qqmlcontextwrapper_p.h>
#include <private/qqmlvaluetypewrapper_p.h>
#include <private/qqmllistwrapper_p.h>
#include <private/qv4scopedvalue_p.h>

#include "qv4domerrors_p.h"
#include "qv4sqlerrors_p.h"

#include <QtCore/qjsonarray.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonvalue.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qdatastream.h>
#include <private/qsimd_p.h>

#include <private/qv4value_inl_p.h>
#include <private/qv4dateobject_p.h>
#include <private/qv4objectiterator_p.h>
#include <private/qv4mm_p.h>
#include <private/qv4objectproto_p.h>
#include <private/qv4globalobject_p.h>
#include <private/qv4regexpobject_p.h>
#include <private/qv4variantobject_p.h>
#include <private/qv4script_p.h>
#include <private/qv4include_p.h>
#include <private/qv4jsonobject_p.h>

Q_DECLARE_METATYPE(QList<int>)


// XXX TODO: Need to check all the global functions will also work in a worker script where the
// QQmlEngine is not available
QT_BEGIN_NAMESPACE

template <typename ReturnType>
ReturnType convertJSValueToVariantType(const QJSValue &value)
{
    return value.toVariant().value<ReturnType>();
}

static void saveJSValue(QDataStream &stream, const void *data)
{
    const QJSValue *jsv = reinterpret_cast<const QJSValue *>(data);
    quint32 isNullOrUndefined = 0;
    if (jsv->isNull())
        isNullOrUndefined |= 0x1;
    if (jsv->isUndefined())
        isNullOrUndefined |= 0x2;
    stream << isNullOrUndefined;
    if (!isNullOrUndefined)
        reinterpret_cast<const QJSValue*>(data)->toVariant().save(stream);
}

static void restoreJSValue(QDataStream &stream, void *data)
{
    QJSValue *jsv = reinterpret_cast<QJSValue*>(data);

    quint32 isNullOrUndefined;
    stream >> isNullOrUndefined;

    if (isNullOrUndefined & 0x1) {
        *jsv = QJSValue(QJSValue::NullValue);
    } else if (isNullOrUndefined & 0x2) {
        *jsv = QJSValue();
    } else {
        QVariant v;
        v.load(stream);
        QJSValuePrivate::setVariant(jsv, v);
    }
}

QV8Engine::QV8Engine(QJSEngine* qq)
    : q(qq)
    , m_engine(0)
    , m_xmlHttpRequestData(0)
    , m_listModelData(0)
{
#ifdef Q_PROCESSOR_X86_32
    if (!qCpuHasFeature(SSE2)) {
        qFatal("This program requires an X86 processor that supports SSE2 extension, at least a Pentium 4 or newer");
    }
#endif

    QML_MEMORY_SCOPE_STRING("QV8Engine::QV8Engine");
    qMetaTypeId<QJSValue>();
    qMetaTypeId<QList<int> >();

    if (!QMetaType::hasRegisteredConverterFunction<QJSValue, QVariantMap>())
        QMetaType::registerConverter<QJSValue, QVariantMap>(convertJSValueToVariantType<QVariantMap>);
    if (!QMetaType::hasRegisteredConverterFunction<QJSValue, QVariantList>())
        QMetaType::registerConverter<QJSValue, QVariantList>(convertJSValueToVariantType<QVariantList>);
    if (!QMetaType::hasRegisteredConverterFunction<QJSValue, QStringList>())
        QMetaType::registerConverter<QJSValue, QStringList>(convertJSValueToVariantType<QStringList>);
    QMetaType::registerStreamOperators(qMetaTypeId<QJSValue>(), saveJSValue, restoreJSValue);

    m_v4Engine = new QV4::ExecutionEngine;
    m_v4Engine->v8Engine = this;

    QV4::QObjectWrapper::initializeBindings(m_v4Engine);
}

QV8Engine::~QV8Engine()
{
    for (int ii = 0; ii < m_extensionData.count(); ++ii)
        delete m_extensionData[ii];
    m_extensionData.clear();

    qt_rem_qmlxmlhttprequest(m_v4Engine, m_xmlHttpRequestData);
    m_xmlHttpRequestData = 0;
    delete m_listModelData;
    m_listModelData = 0;

    delete m_v4Engine;
}

QNetworkAccessManager *QV8Engine::networkAccessManager()
{
    return QQmlEnginePrivate::get(m_engine)->getNetworkAccessManager();
}

const QSet<QString> &QV8Engine::illegalNames() const
{
    return m_illegalNames;
}

QQmlContextData *QV8Engine::callingContext()
{
    return QV4::QmlContextWrapper::callingContext(m_v4Engine);
}

void QV8Engine::initializeGlobal()
{
    QV4::Scope scope(m_v4Engine);
    QV4::GlobalExtensions::init(m_engine, m_v4Engine->globalObject());

    QQmlLocale::registerStringLocaleCompare(m_v4Engine);
    QQmlDateExtension::registerExtension(m_v4Engine);
    QQmlNumberExtension::registerExtension(m_v4Engine);

    qt_add_domexceptions(m_v4Engine);
    m_xmlHttpRequestData = qt_add_qmlxmlhttprequest(m_v4Engine);

    qt_add_sqlexceptions(m_v4Engine);

    {
        for (uint i = 0; i < m_v4Engine->globalObject()->internalClass()->size; ++i) {
            if (m_v4Engine->globalObject()->internalClass()->nameMap.at(i))
                m_illegalNames.insert(m_v4Engine->globalObject()->internalClass()->nameMap.at(i)->string);
        }
    }

    {
#define FREEZE_SOURCE "(function freeze_recur(obj) { "\
                      "    if (Qt.isQtObject(obj)) return;"\
                      "    if (obj != Function.connect && obj != Function.disconnect && "\
                      "        obj instanceof Object) {"\
                      "        var properties = Object.getOwnPropertyNames(obj);"\
                      "        for (var prop in properties) { "\
                      "            if (prop == \"connect\" || prop == \"disconnect\") {"\
                      "                Object.freeze(obj[prop]); "\
                      "                continue;"\
                      "            }"\
                      "            freeze_recur(obj[prop]);"\
                      "        }"\
                      "    }"\
                      "    if (obj instanceof Object) {"\
                      "        Object.freeze(obj);"\
                      "    }"\
                      "})"

        QV4::ScopedFunctionObject result(scope, QV4::Script::evaluate(m_v4Engine, QString::fromUtf8(FREEZE_SOURCE), 0));
        Q_ASSERT(!!result);
        m_freezeObject.set(scope.engine, result);
#undef FREEZE_SOURCE
    }
}

void QV8Engine::freezeObject(const QV4::Value &value)
{
    QV4::Scope scope(m_v4Engine);
    QV4::ScopedFunctionObject f(scope, m_freezeObject.value());
    QV4::ScopedCallData callData(scope, 1);
    callData->args[0] = value;
    callData->thisObject = m_v4Engine->globalObject();
    f->call(callData);
}

struct QV8EngineRegistrationData
{
    QV8EngineRegistrationData() : extensionCount(0) {}

    QMutex mutex;
    int extensionCount;
};
Q_GLOBAL_STATIC(QV8EngineRegistrationData, registrationData);

QMutex *QV8Engine::registrationMutex()
{
    return &registrationData()->mutex;
}

int QV8Engine::registerExtension()
{
    return registrationData()->extensionCount++;
}

void QV8Engine::setExtensionData(int index, Deletable *data)
{
    if (m_extensionData.count() <= index)
        m_extensionData.resize(index + 1);

    if (m_extensionData.at(index))
        delete m_extensionData.at(index);

    m_extensionData[index] = data;
}

void QV8Engine::initQmlGlobalObject()
{
    initializeGlobal();
    QV4::Scope scope(m_v4Engine);
    QV4::ScopedValue v(scope, m_v4Engine->globalObject());
    freezeObject(v);
}

void QV8Engine::setEngine(QQmlEngine *engine)
{
    m_engine = engine;
    initQmlGlobalObject();
}

QV4::ReturnedValue QV8Engine::global()
{
    return m_v4Engine->globalObject()->asReturnedValue();
}

void QV8Engine::startTimer(const QString &timerName)
{
    if (!m_time.isValid())
        m_time.start();
    m_startedTimers[timerName] = m_time.elapsed();
}

qint64 QV8Engine::stopTimer(const QString &timerName, bool *wasRunning)
{
    if (!m_startedTimers.contains(timerName)) {
        *wasRunning = false;
        return 0;
    }
    *wasRunning = true;
    qint64 startedAt = m_startedTimers.take(timerName);
    return m_time.elapsed() - startedAt;
}

int QV8Engine::consoleCountHelper(const QString &file, quint16 line, quint16 column)
{
    const QString key = file + QString::number(line) + QString::number(column);
    int number = m_consoleCount.value(key, 0);
    number++;
    m_consoleCount.insert(key, number);
    return number;
}

QT_END_NAMESPACE

