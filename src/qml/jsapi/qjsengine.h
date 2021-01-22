/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QJSENGINE_H
#define QJSENGINE_H

#include <QtCore/qmetatype.h>

#include <QtCore/qvariant.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qobject.h>
#include <QtQml/qjsvalue.h>

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

    QJSValue evaluate(const QString &program, const QString &fileName = QString(), int lineNumber = 1);

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
    inline T fromScriptValue(const QJSValue &value)
    {
        return qjsvalue_cast<T>(value);
    }

    void collectGarbage();

#if QT_DEPRECATED_SINCE(5, 6)
    QT_DEPRECATED void installTranslatorFunctions(const QJSValue &object = QJSValue());
#endif

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

    QString uiLanguage() const;
    void setUiLanguage(const QString &language);

Q_SIGNALS:
    void uiLanguageChanged();

private:
    QJSValue create(int type, const void *ptr);

    static bool convertV2(const QJSValue &value, int type, void *ptr);

    friend inline bool qjsvalue_cast_helper(const QJSValue &, int, void *);

protected:
    QJSEngine(QJSEnginePrivate &dd, QObject *parent = nullptr);

private:
    QV4::ExecutionEngine *m_v4Engine;
    Q_DISABLE_COPY(QJSEngine)
    Q_DECLARE_PRIVATE(QJSEngine)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QJSEngine::Extensions)

inline bool qjsvalue_cast_helper(const QJSValue &value, int type, void *ptr)
{
    return QJSEngine::convertV2(value, type, ptr);
}

template<typename T>
T qjsvalue_cast(const QJSValue &value)
{
    T t;
    const int id = qMetaTypeId<T>();

    if (qjsvalue_cast_helper(value, id, &t))
        return t;
    else if (value.isVariant())
        return qvariant_cast<T>(value.toVariant());

    return T();
}

template <>
inline QVariant qjsvalue_cast<QVariant>(const QJSValue &value)
{
    return value.toVariant();
}

Q_QML_EXPORT QJSEngine *qjsEngine(const QObject *);

QT_END_NAMESPACE

#endif // QJSENGINE_H
