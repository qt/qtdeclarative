/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QJSENGINE_H
#define QJSENGINE_H

#include <QtCore/qmetatype.h>

#include <QtCore/qvariant.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qobject.h>
#include <QtQml/qjsvalue.h>
#include <QtQml/qjsmanagedvalue.h>
#include <QtQml/qqmldebug.h>

QT_BEGIN_NAMESPACE


template <typename T>
inline T qjsvalue_cast(const QJSValue &);

class QJSEnginePrivate;
class Q_QML_EXPORT QJSEngine
    : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString uiLanguage READ uiLanguage WRITE setUiLanguage NOTIFY uiLanguageChanged)
public:
    QJSEngine();
    explicit QJSEngine(QObject *parent);
    ~QJSEngine() override;

    QJSValue globalObject() const;

    QJSValue evaluate(const QString &program, const QString &fileName = QString(), int lineNumber = 1, QStringList *exceptionStackTrace = nullptr);

    QJSValue importModule(const QString &fileName);

    QJSValue newObject();
    QJSValue newArray(uint length = 0);

    QJSValue newQObject(QObject *object);

    QJSValue newQMetaObject(const QMetaObject* metaObject);

    template <typename T>
    QJSValue newQMetaObject()
    {
        return newQMetaObject(&T::staticMetaObject);
    }

    QJSValue newErrorObject(QJSValue::ErrorType errorType, const QString &message = QString());

    template <typename T>
    inline QJSValue toScriptValue(const T &value)
    {
        return create(qMetaTypeId<T>(), &value);
    }

    template <typename T>
    inline QJSManagedValue toManagedValue(const T &value)
    {
        return createManaged(QMetaType::fromType<T>(), &value);
    }

    template <typename T>
    inline T fromScriptValue(const QJSValue &value)
    {
        return qjsvalue_cast<T>(value);
    }

    template <typename T>
    inline T fromManagedValue(const QJSManagedValue &value)
    {
        return qjsvalue_cast<T>(value);
    }

    void collectGarbage();

    enum ObjectOwnership { CppOwnership, JavaScriptOwnership };
    static void setObjectOwnership(QObject *, ObjectOwnership);
    static ObjectOwnership objectOwnership(QObject *);

    enum Extension {
        TranslationExtension = 0x1,
        ConsoleExtension = 0x2,
        GarbageCollectionExtension = 0x4,
        AllExtensions = 0xffffffff
    };
    Q_DECLARE_FLAGS(Extensions, Extension)

    void installExtensions(Extensions extensions, const QJSValue &object = QJSValue());

    void setInterrupted(bool interrupted);
    bool isInterrupted() const;

    QV4::ExecutionEngine *handle() const { return m_v4Engine; }

    void throwError(const QString &message);
    void throwError(QJSValue::ErrorType errorType, const QString &message = QString());
    void throwError(const QJSValue &error);
    bool hasError() const;
    QJSValue catchError();

    QString uiLanguage() const;
    void setUiLanguage(const QString &language);

Q_SIGNALS:
    void uiLanguageChanged();

private:
    QJSManagedValue createManaged(QMetaType type, const void *ptr);
    QJSValue create(int type, const void *ptr);

    static bool convertManaged(const QJSManagedValue &value, int type, void *ptr);
    static bool convertV2(const QJSValue &value, int type, void *ptr);

    template<typename T>
    friend inline T qjsvalue_cast(const QJSValue &);

    template<typename T>
    friend inline T qjsvalue_cast(const QJSManagedValue &);

protected:
    QJSEngine(QJSEnginePrivate &dd, QObject *parent = nullptr);

private:
    QV4::ExecutionEngine *m_v4Engine;
    Q_DISABLE_COPY(QJSEngine)
    Q_DECLARE_PRIVATE(QJSEngine)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QJSEngine::Extensions)

template<typename T>
T qjsvalue_cast(const QJSValue &value)
{
    T t;
    const int id = qMetaTypeId<T>();

    if (QJSEngine::convertV2(value, id, &t))
        return t;
    else if (value.isVariant())
        return qvariant_cast<T>(value.toVariant());

    return T();
}

template<typename T>
T qjsvalue_cast(const QJSManagedValue &value)
{
    {
        T t;
        if (QJSEngine::convertManaged(value, qMetaTypeId<T>(), &t))
            return t;
    }

    return qvariant_cast<T>(value.toVariant());
}

template <>
inline QVariant qjsvalue_cast<QVariant>(const QJSValue &value)
{
    return value.toVariant();
}

template <>
inline QVariant qjsvalue_cast<QVariant>(const QJSManagedValue &value)
{
    return value.toVariant();
}

Q_QML_EXPORT QJSEngine *qjsEngine(const QObject *);

QT_END_NAMESPACE

#endif // QJSENGINE_H
