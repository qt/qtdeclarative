// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    bool registerModule(const QString &moduleName, const QJSValue &value);

    QJSValue newObject();
    QJSValue newSymbol(const QString &name);
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
        return create(QMetaType::fromType<T>(), &value);
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

    template <typename T>
    inline T fromVariant(const QVariant &value)
    {
        if constexpr (std::is_same_v<T, QVariant>)
            return value;

        const QMetaType targetType = QMetaType::fromType<T>();
        if (value.metaType() == targetType)
            return *reinterpret_cast<const T *>(value.constData());

        if constexpr (std::is_same_v<T,std::remove_const_t<std::remove_pointer_t<T>> const *>) {
            using nonConstT = std::remove_const_t<std::remove_pointer_t<T>> *;
            const QMetaType nonConstTargetType = QMetaType::fromType<nonConstT>();
            if (value.metaType() == nonConstTargetType)
                return *reinterpret_cast<const nonConstT *>(value.constData());
        }

        {
            T t{};
            if (convertVariant(value, targetType, &t))
                return t;

            QMetaType::convert(value.metaType(), value.constData(), targetType, &t);
            return t;
        }
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
    QJSValue create(QMetaType type, const void *ptr);
#if QT_VERSION < QT_VERSION_CHECK(7,0,0)
    QJSValue create(int id, const void *ptr); // only there for BC reasons
#endif

    static bool convertManaged(const QJSManagedValue &value, int type, void *ptr);
    static bool convertManaged(const QJSManagedValue &value, QMetaType type, void *ptr);
    static bool convertV2(const QJSValue &value, int type, void *ptr);
    static bool convertV2(const QJSValue &value, QMetaType metaType, void *ptr);
    bool convertVariant(const QVariant &value, QMetaType metaType, void *ptr);

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
    if (QJSEngine::convertV2(value, QMetaType::fromType<T>(), &t))
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
        if (QJSEngine::convertManaged(value, QMetaType::fromType<T>(), &t))
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
