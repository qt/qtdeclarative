// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlinfo.h"

#include "qqmldata_p.h"
#include "qqmlmetatype_p.h"
#include "qqmlengine_p.h"
#include "qqmlsourcecoordinate_p.h"

#include <QCoreApplication>

QT_BEGIN_NAMESPACE

/*!
    \class QQmlInfo
    \inmodule QtQml
    \brief The QQmlInfo class allows logging of QML-related messages.

    QQmlInfo is an opaque handle for QML-related diagnostic messages. You can
    use the \c{<<} operator to add content to the message. When the QQmlInfo
    object is destroyed, it prints the resulting message along with information
    on the context.

    \sa qmlDebug, qmlInfo, qmlWarning
*/

/*!
    \fn QQmlInfo qmlDebug(const QObject *object)
    \relates QQmlInfo
    \since 5.9

    Prints debug messages that include the file and line number for the
    specified QML \a object.

//! [qqmlinfo-desc]
    When QML types produce logging messages, it improves traceability
    if they include the QML file and line number on which the
    particular instance was instantiated.

    To include the file and line number, an object must be passed.  If
    the file and line number is not available for that instance
    (either it was not instantiated by the QML engine or location
    information is disabled), "unknown location" will be used instead.
//! [qqmlinfo-desc]

    For example,

    \code
    qmlDebug(object) << "Internal state: 42";
    \endcode

    prints

    \badcode
    QML MyCustomType (unknown location): Internal state: 42
    \endcode

    \sa qmlInfo, qmlWarning
*/

/*!
    \fn QQmlInfo qmlInfo(const QObject *object)
    \relates QQmlInfo

    Prints informational messages that include the file and line number for the
    specified QML \a object.

    \include qqmlinfo.cpp qqmlinfo-desc

    For example,

    \code
    qmlInfo(object) << tr("component property is a write-once property");
    \endcode

    prints

    \badcode
    QML MyCustomType (unknown location): component property is a write-once property
    \endcode

    \note In versions prior to Qt 5.9, qmlInfo reported messages using a warning
    QtMsgType. For Qt 5.9 and above, qmlInfo uses an info QtMsgType. To send
    warnings, use qmlWarning.

    \sa qmlDebug, qmlWarning
*/

/*!
    \fn QQmlInfo qmlWarning(const QObject *object)
    \relates QQmlInfo
    \since 5.9

    Prints warning messages that include the file and line number for the
    specified QML \a object.

    \include qqmlinfo.cpp qqmlinfo-desc

    For example,

    \code
    qmlInfo(object) << tr("property cannot be set to 0");
    \endcode

    prints

    \badcode
    QML MyCustomType (unknown location): property cannot be set to 0
    \endcode

    \sa qmlDebug, qmlInfo
*/

/*!
    \fn QQmlInfo qmlDebug(const QObject *object, const QQmlError &error)
    \internal
*/

/*!
    \fn QQmlInfo qmlDebug(const QObject *object, const QList<QQmlError> &errors)
    \internal
*/

/*!
    \fn QQmlInfo qmlInfo(const QObject *object, const QQmlError &error)
    \internal
*/

/*!
    \fn QQmlInfo qmlInfo(const QObject *object, const QList<QQmlError> &errors)
    \internal
*/

/*!
    \fn QQmlInfo qmlWarning(const QObject *object, const QQmlError &error)
    \internal
*/

/*!
    \fn QQmlInfo qmlWarning(const QObject *object, const QList<QQmlError> &errors)
    \internal
*/

class QQmlInfoPrivate
{
public:
    QQmlInfoPrivate(QtMsgType type)
        : ref (1)
        , msgType(type)
        , object(nullptr)
    {}

    int ref;
    QtMsgType msgType;
    const QObject *object;
    QString buffer;
    QList<QQmlError> errors;
};

QQmlInfo::QQmlInfo(QQmlInfoPrivate *p)
: QDebug(&p->buffer), d(p)
{
    nospace();
}

QQmlInfo::QQmlInfo(const QQmlInfo &other)
: QDebug(other), d(other.d)
{
    d->ref++;
}

QQmlInfo::~QQmlInfo()
{
    if (0 == --d->ref) {
        QList<QQmlError> errors = d->errors;

        QQmlEngine *engine = nullptr;

        if (!d->buffer.isEmpty()) {
            QQmlError error;
            error.setMessageType(d->msgType);

            QObject *object = const_cast<QObject *>(d->object);

            if (object) {
                // Some objects (e.g. like attached objects created in C++) won't have an associated engine,
                // but we can still try to look for a parent object that does.
                QObject *objectWithEngine = object;
                while (objectWithEngine) {
                    engine = qmlEngine(objectWithEngine);
                    if (engine)
                        break;
                    objectWithEngine = objectWithEngine->parent();
                }

                if (!objectWithEngine || objectWithEngine == object) {
                    d->buffer.prepend(QLatin1String("QML ") + QQmlMetaType::prettyTypeName(object) + QLatin1String(": "));
                } else {
                    d->buffer.prepend(QLatin1String("QML ") + QQmlMetaType::prettyTypeName(objectWithEngine)
                        + QLatin1String(" (parent or ancestor of ") + QQmlMetaType::prettyTypeName(object) + QLatin1String("): "));
                }

                QQmlData *ddata = QQmlData::get(objectWithEngine ? objectWithEngine : object, false);
                if (ddata && ddata->outerContext) {
                    error.setUrl(ddata->outerContext->url());
                    error.setLine(qmlConvertSourceCoordinate<quint16, int>(ddata->lineNumber));
                    error.setColumn(qmlConvertSourceCoordinate<quint16, int>(ddata->columnNumber));
                }
            }

            error.setDescription(d->buffer);

            errors.prepend(error);
        }

        QQmlEnginePrivate::warning(engine, errors);

        delete d;
    }
}

#define MESSAGE_FUNCS(FuncName, MessageLevel) \
    QQmlInfo FuncName(const QObject *me) \
    { \
        QQmlInfoPrivate *d = new QQmlInfoPrivate(MessageLevel); \
        d->object = me; \
        return QQmlInfo(d); \
    } \
    QQmlInfo FuncName(const QObject *me, const QQmlError &error) \
    { \
        QQmlInfoPrivate *d = new QQmlInfoPrivate(MessageLevel); \
        d->object = me; \
        d->errors << error; \
        return QQmlInfo(d); \
    } \
    QQmlInfo FuncName(const QObject *me, const QList<QQmlError> &errors) \
    { \
        QQmlInfoPrivate *d = new QQmlInfoPrivate(MessageLevel); \
        d->object = me; \
        d->errors = errors; \
        return QQmlInfo(d); \
    }

MESSAGE_FUNCS(qmlDebug, QtMsgType::QtDebugMsg)
MESSAGE_FUNCS(qmlInfo, QtMsgType::QtInfoMsg)
MESSAGE_FUNCS(qmlWarning, QtMsgType::QtWarningMsg)

QT_END_NAMESPACE
