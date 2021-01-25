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

#ifndef QQMLPRIVATE_H
#define QQMLPRIVATE_H

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

#include <functional>
#include <type_traits>

#include <QtQml/qtqmlglobal.h>
#include <QtQml/qqmlparserstatus.h>
#include <QtQml/qqmllist.h>
#include <QtQml/qqmlpropertyvaluesource.h>
#include <QtQml/qjsvalue.h>

#include <QtCore/qglobal.h>
#include <QtCore/qvariant.h>
#include <QtCore/qurl.h>
#include <QtCore/qpointer.h>
#include <QtCore/qversionnumber.h>

#include <QtCore/qmetaobject.h>
#include <QtCore/qmetacontainer.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

class QQmlPropertyValueInterceptor;
class QQmlContext;

namespace QQmlPrivate {
struct CachedQmlUnit;
template<typename A>
using QQmlAttachedPropertiesFunc = A *(*)(QObject *);
}

namespace QV4 {
struct ExecutionEngine;
namespace CompiledData {
struct Unit;
struct CompilationUnit;
}
}
namespace QmlIR {
struct Document;
typedef void (*IRLoaderFunction)(Document *, const QQmlPrivate::CachedQmlUnit *);
}

using QQmlAttachedPropertiesFunc = QQmlPrivate::QQmlAttachedPropertiesFunc<QObject>;

inline size_t qHash(QQmlAttachedPropertiesFunc func, size_t seed = 0)
{
    return qHash(quintptr(func), seed);
}

template <typename TYPE>
class QQmlTypeInfo
{
public:
    enum {
        hasAttachedProperties = 0
    };
};


class QJSEngine;
class QQmlEngine;
class QQmlCustomParser;
class QQmlTypeNotAvailable;

template<class T>
QQmlCustomParser *qmlCreateCustomParser()
{
    return nullptr;
}

namespace QQmlPrivate
{
    void Q_QML_EXPORT qdeclarativeelement_destructor(QObject *);
    template<typename T>
    class QQmlElement final : public T
    {
    public:
        ~QQmlElement() override {
            QQmlPrivate::qdeclarativeelement_destructor(this);
        }
        static void operator delete(void *ptr) {
            // We allocate memory from this class in QQmlType::create
            // along with some additional memory.
            // So we override the operator delete in order to avoid the
            // sized operator delete to be called with a different size than
            // the size that was allocated.
            ::operator delete (ptr);
        }
        static void operator delete(void *, void *) {
            // Deliberately empty placement delete operator.
            // Silences MSVC warning C4291: no matching operator delete found
        }
    };

    enum class ConstructionMode
    {
        None,
        Constructor,
        Factory,
        FactoryWrapper
    };

    template<typename T, typename WrapperT = T, typename = std::void_t<>>
    struct HasSingletonFactory
    {
        static constexpr bool value = false;
    };

    template<typename T, typename WrapperT>
    struct HasSingletonFactory<T, WrapperT, std::void_t<decltype(WrapperT::create(
                                                               static_cast<QQmlEngine *>(nullptr),
                                                               static_cast<QJSEngine *>(nullptr)))>>
    {
        static constexpr bool value = std::is_same_v<
            decltype(WrapperT::create(static_cast<QQmlEngine *>(nullptr),
                               static_cast<QJSEngine *>(nullptr))), T *>;
    };

    template<typename T, typename WrapperT>
    constexpr ConstructionMode constructionMode()
    {
        if constexpr (!std::is_base_of<QObject, T>::value)
            return ConstructionMode::None;
        if constexpr (!std::is_same_v<T, WrapperT> && HasSingletonFactory<T, WrapperT>::value)
            return ConstructionMode::FactoryWrapper;
        if constexpr (std::is_default_constructible<T>::value)
            return ConstructionMode::Constructor;
        if constexpr (HasSingletonFactory<T>::value)
            return ConstructionMode::Factory;

        return ConstructionMode::None;
    }

    template<typename T>
    void createInto(void *memory, void *) { new (memory) QQmlElement<T>; }

    template<typename T, typename WrapperT, ConstructionMode Mode>
    QObject *createSingletonInstance(QQmlEngine *q, QJSEngine *j)
    {
        Q_UNUSED(q);
        Q_UNUSED(j);
        if constexpr (Mode == ConstructionMode::Constructor)
            return new T;
        else if constexpr (Mode == ConstructionMode::Factory)
            return T::create(q, j);
        else if constexpr (Mode == ConstructionMode::FactoryWrapper)
            return WrapperT::create(q, j);
        else
            return nullptr;
    }

    template<typename T>
    QObject *createParent(QObject *p) { return new T(p); }

    using CreateIntoFunction = void (*)(void *, void *);
    using CreateSingletonFunction = QObject *(*)(QQmlEngine *, QJSEngine *);
    using CreateParentFunction = QObject *(*)(QObject *);
    using CreateValueTypeFunction = QVariant (*)(const QJSValue &);

    template<typename T, typename WrapperT = T, ConstructionMode Mode = constructionMode<T, WrapperT>()>
    struct Constructors;

    template<typename T, typename WrapperT>
    struct Constructors<T, WrapperT, ConstructionMode::Constructor>
    {
        static constexpr CreateIntoFunction createInto
                = QQmlPrivate::createInto<T>;
        static constexpr CreateSingletonFunction createSingletonInstance
                = QQmlPrivate::createSingletonInstance<T, WrapperT, ConstructionMode::Constructor>;
    };

    template<typename T, typename WrapperT>
    struct Constructors<T, WrapperT, ConstructionMode::None>
    {
        static constexpr CreateIntoFunction createInto = nullptr;
        static constexpr CreateSingletonFunction createSingletonInstance = nullptr;
    };

    template<typename T, typename WrapperT>
    struct Constructors<T, WrapperT, ConstructionMode::Factory>
    {
        static constexpr CreateIntoFunction createInto = nullptr;
        static constexpr CreateSingletonFunction createSingletonInstance
                = QQmlPrivate::createSingletonInstance<T, WrapperT, ConstructionMode::Factory>;
    };

    template<typename T, typename WrapperT>
    struct Constructors<T, WrapperT, ConstructionMode::FactoryWrapper>
    {
        static constexpr CreateIntoFunction createInto = nullptr;
        static constexpr CreateSingletonFunction createSingletonInstance
                = QQmlPrivate::createSingletonInstance<T, WrapperT, ConstructionMode::FactoryWrapper>;
    };

    template<typename T,
             bool IsObject = std::is_base_of<QObject, T>::value,
             bool IsGadget = QtPrivate::IsGadgetHelper<T>::IsRealGadget>
    struct ExtendedType;

    template<typename T>
    struct ExtendedType<T, false, false>
    {
        static constexpr const CreateParentFunction createParent = nullptr;
        static const QMetaObject *staticMetaObject() { return nullptr; }
    };

    // If it's a QObject, we actually want an error if the ctor or the metaobject is missing.
    template<typename T>
    struct ExtendedType<T, true, false>
    {
        static constexpr const CreateParentFunction createParent = QQmlPrivate::createParent<T>;
        static const QMetaObject *staticMetaObject() { return &T::staticMetaObject; }
    };

    // If it's a Q_GADGET, we don't want the ctor.
    template<typename T>
    struct ExtendedType<T, false, true>
    {
        static constexpr const CreateParentFunction createParent = nullptr;
        static const QMetaObject *staticMetaObject() { return &T::staticMetaObject; }
    };

    template<typename F, typename Result = void>
    struct ValueTypeFactory
    {
        static constexpr const Result (*create)(const QJSValue &) = nullptr;
    };

    template<typename F>
    struct ValueTypeFactory<F, std::void_t<decltype(F::create(QJSValue()))>>
    {
        static decltype(F::create(QJSValue())) create(const QJSValue &params)
        {
            return F::create(params);
        }
    };

    template<typename T, typename F,
             bool HasCtor = std::is_constructible_v<T, QJSValue>,
             bool HasFactory = std::is_constructible_v<
                 QVariant, decltype(ValueTypeFactory<F>::create(QJSValue()))>>
    struct ValueType;

    template<typename T, typename F>
    struct ValueType<T, F, false, false>
    {
        static constexpr const CreateValueTypeFunction create = nullptr;
    };

    template<typename T, typename F, bool HasCtor>
    struct ValueType<T, F, HasCtor, true>
    {
        static QVariant create(const QJSValue &params)
        {
            return F::create(params);
        }
    };

    template<typename T, typename F>
    struct ValueType<T, F, true, false>
    {
        static QVariant create(const QJSValue &params)
        {
            return QVariant::fromValue(T(params));
        }
    };

    template<class From, class To, int N>
    struct StaticCastSelectorClass
    {
        static inline int cast() { return -1; }
    };

    template<class From, class To>
    struct StaticCastSelectorClass<From, To, sizeof(int)>
    {
        static inline int cast() { return int(reinterpret_cast<quintptr>(static_cast<To *>(reinterpret_cast<From *>(0x10000000)))) - 0x10000000; }
    };

    template<class From, class To>
    struct StaticCastSelector
    {
        typedef int yes_type;
        typedef char no_type;

        static yes_type checkType(To *);
        static no_type checkType(...);

        static inline int cast()
        {
            return StaticCastSelectorClass<From, To, sizeof(checkType(reinterpret_cast<From *>(0)))>::cast();
        }
    };

    // You can prevent subclasses from using the same attached type by specialzing this.
    // This is reserved for internal types, though.
    template<class T, class A>
    struct OverridableAttachedType
    {
        using Type = A;
    };

    template<class T, class = std::void_t<>, bool OldStyle = QQmlTypeInfo<T>::hasAttachedProperties>
    struct QmlAttached
    {
        using Type = void;
        using Func = QQmlAttachedPropertiesFunc<QObject>;
        static const QMetaObject *staticMetaObject() { return nullptr; }
        static Func attachedPropertiesFunc() { return nullptr; }
    };

    // Defined inline via QML_ATTACHED
    template<class T>
    struct QmlAttached<T, std::void_t<typename OverridableAttachedType<T, typename T::QmlAttachedType>::Type>, false>
    {
        // Normal attached properties
        template <typename Parent, typename Attached>
        struct Properties
        {
            using Func = QQmlAttachedPropertiesFunc<Attached>;
            static const QMetaObject *staticMetaObject() { return &Attached::staticMetaObject; }
            static Func attachedPropertiesFunc() { return Parent::qmlAttachedProperties; }
        };

        // Disabled via OverridableAttachedType
        template<typename Parent>
        struct Properties<Parent, void>
        {
            using Func = QQmlAttachedPropertiesFunc<QObject>;
            static const QMetaObject *staticMetaObject() { return nullptr; };
            static Func attachedPropertiesFunc() { return nullptr; };
        };

        using Type = typename OverridableAttachedType<T, typename T::QmlAttachedType>::Type;
        using Func = typename Properties<T, Type>::Func;

        static const QMetaObject *staticMetaObject()
        {
            return Properties<T, Type>::staticMetaObject();
        }

        static Func attachedPropertiesFunc()
        {
            return Properties<T, Type>::attachedPropertiesFunc();
        }
    };

    // Separately defined via QQmlTypeInfo
    template<class T>
    struct QmlAttached<T, std::void_t<decltype(T::qmlAttachedProperties)>, true>
    {
        using Type = typename std::remove_pointer<decltype(T::qmlAttachedProperties(nullptr))>::type;
        using Func = QQmlAttachedPropertiesFunc<Type>;

        static const QMetaObject *staticMetaObject() { return &Type::staticMetaObject; }
        static Func attachedPropertiesFunc() { return T::qmlAttachedProperties; }
    };

    // This is necessary because both the type containing a default template parameter and the type
    // instantiating the template need to have access to the default template parameter type. In
    // this case that's T::QmlAttachedType. The QML_FOREIGN macro needs to befriend specific other
    // types. Therefore we need some kind of "accessor". Because of compiler bugs in gcc and clang,
    // we cannot befriend attachedPropertiesFunc() directly. Wrapping the actual access into another
    // struct "fixes" that. For convenience we still want the free standing functions in addition.
    template<class T>
    struct QmlAttachedAccessor
    {
        static QQmlAttachedPropertiesFunc<QObject> attachedPropertiesFunc()
        {
            return QQmlAttachedPropertiesFunc<QObject>(QmlAttached<T>::attachedPropertiesFunc());
        }

        static const QMetaObject *staticMetaObject()
        {
            return QmlAttached<T>::staticMetaObject();
        }
    };

    template<typename T>
    inline QQmlAttachedPropertiesFunc<QObject> attachedPropertiesFunc()
    {
        return QmlAttachedAccessor<T>::attachedPropertiesFunc();
    }

    template<typename T>
    inline const QMetaObject *attachedPropertiesMetaObject()
    {
        return QmlAttachedAccessor<T>::staticMetaObject();
    }

    enum AutoParentResult { Parented, IncompatibleObject, IncompatibleParent };
    typedef AutoParentResult (*AutoParentFunction)(QObject *object, QObject *parent);

    struct RegisterType {
        int structVersion;

        QMetaType typeId;
        QMetaType listId;
        int objectSize;
        // The second parameter of create is for userdata
        void (*create)(void *, void *);
        void *userdata;
        QString noCreationReason;

        QVariant (*createValueType)(const QJSValue &);

        const char *uri;
        QTypeRevision version;
        const char *elementName;
        const QMetaObject *metaObject;

        QQmlAttachedPropertiesFunc<QObject> attachedPropertiesFunction;
        const QMetaObject *attachedPropertiesMetaObject;

        int parserStatusCast;
        int valueSourceCast;
        int valueInterceptorCast;

        QObject *(*extensionObjectCreate)(QObject *);
        const QMetaObject *extensionMetaObject;

        QQmlCustomParser *customParser;

        QTypeRevision revision;
        // If this is extended ensure "version" is bumped!!!
    };

    struct RegisterTypeAndRevisions {
        int structVersion;

        QMetaType typeId;
        QMetaType listId;
        int objectSize;
        void (*create)(void *, void *);
        void *userdata;

        QVariant (*createValueType)(const QJSValue &);

        const char *uri;
        QTypeRevision version;

        const QMetaObject *metaObject;
        const QMetaObject *classInfoMetaObject;

        QQmlAttachedPropertiesFunc<QObject> attachedPropertiesFunction;
        const QMetaObject *attachedPropertiesMetaObject;

        int parserStatusCast;
        int valueSourceCast;
        int valueInterceptorCast;

        QObject *(*extensionObjectCreate)(QObject *);
        const QMetaObject *extensionMetaObject;

        QQmlCustomParser *(*customParserFactory)();
        QVector<int> *qmlTypeIds;
    };

    struct RegisterInterface {
        int structVersion;

        QMetaType typeId;
        QMetaType listId;

        const char *iid;

        const char *uri;
        QTypeRevision version;
    };

    struct RegisterAutoParent {
        int structVersion;

        AutoParentFunction function;
    };

    struct RegisterSingletonType {
        int structVersion;

        const char *uri;
        QTypeRevision version;
        const char *typeName;

        std::function<QJSValue(QQmlEngine *, QJSEngine *)> scriptApi;
        std::function<QObject*(QQmlEngine *, QJSEngine *)> qObjectApi;

        const QMetaObject *instanceMetaObject;
        QMetaType typeId;

        QObject *(*extensionObjectCreate)(QObject *);
        const QMetaObject *extensionMetaObject;

        QTypeRevision revision;
    };

    struct RegisterSingletonTypeAndRevisions {
        int structVersion;
        const char *uri;
        QTypeRevision version;

        std::function<QObject*(QQmlEngine *, QJSEngine *)> qObjectApi;

        const QMetaObject *instanceMetaObject;
        const QMetaObject *classInfoMetaObject;

        QMetaType typeId;

        QObject *(*extensionObjectCreate)(QObject *);
        const QMetaObject *extensionMetaObject;

        QVector<int> *qmlTypeIds;
    };

    struct RegisterCompositeType {
        int structVersion;
        QUrl url;
        const char *uri;
        QTypeRevision version;
        const char *typeName;
    };

    struct RegisterCompositeSingletonType {
        int structVersion;
        QUrl url;
        const char *uri;
        QTypeRevision version;
        const char *typeName;
    };

    struct RegisterSequentialContainer {
        int structVersion;
        const char *uri;
        QTypeRevision version;
        const char *typeName;
        QMetaType typeId;
        QMetaSequence metaSequence;
        QTypeRevision revision;
    };

    struct RegisterSequentialContainerAndRevisions {
        int structVersion;
        const char *uri;
        QTypeRevision version;

        const QMetaObject *classInfoMetaObject;
        QMetaType typeId;
        QMetaSequence metaSequence;

        QVector<int> *qmlTypeIds;
    };

    struct Q_QML_EXPORT AOTCompiledContext {
        QQmlContext *qmlContext;
        QObject *qmlScopeObject;
        QJSEngine *engine;
        QV4::CompiledData::CompilationUnit *compilationUnit;

        QJSValue jsMetaType(int index) const;
        void setInstructionPointer(int offset) const;
    };

    struct AOTCompiledFunction {
        int index;
        QMetaType returnType;
        QList<QMetaType> argumentTypes;
        void (*functionPtr)(const AOTCompiledContext *context, void *resultPtr, void **arguments);
    };

    struct CachedQmlUnit {
        const QV4::CompiledData::Unit *qmlData;
        const AOTCompiledFunction *aotCompiledFunctions;
        void *unused2;
    };

    typedef const CachedQmlUnit *(*QmlUnitCacheLookupFunction)(const QUrl &url);
    struct RegisterQmlUnitCacheHook {
        int structVersion;
        QmlUnitCacheLookupFunction lookupCachedQmlUnit;
    };

    enum RegistrationType {
        TypeRegistration       = 0,
        InterfaceRegistration  = 1,
        AutoParentRegistration = 2,
        SingletonRegistration  = 3,
        CompositeRegistration  = 4,
        CompositeSingletonRegistration = 5,
        QmlUnitCacheHookRegistration = 6,
        TypeAndRevisionsRegistration = 7,
        SingletonAndRevisionsRegistration = 8,
        SequentialContainerRegistration = 9,
        SequentialContainerAndRevisionsRegistration = 10,
    };

    int Q_QML_EXPORT qmlregister(RegistrationType, void *);
    void Q_QML_EXPORT qmlunregister(RegistrationType, quintptr);
    struct Q_QML_EXPORT SingletonFunctor
    {
        QObject *operator()(QQmlEngine *, QJSEngine *);

        QPointer<QObject> m_object;
        bool alreadyCalled = false;
    };

    static int indexOfOwnClassInfo(const QMetaObject *metaObject, const char *key)
    {
        if (!metaObject || !key)
            return -1;

        const int offset = metaObject->classInfoOffset();
        for (int i = metaObject->classInfoCount() + offset - 1; i >= offset; --i)
            if (qstrcmp(key, metaObject->classInfo(i).name()) == 0) {
                return i;
        }
        return -1;
    }

    inline const char *classInfo(const QMetaObject *metaObject, const char *key)
    {
        return metaObject->classInfo(indexOfOwnClassInfo(metaObject, key)).value();
    }

    inline QTypeRevision revisionClassInfo(const QMetaObject *metaObject, const char *key,
                                       QTypeRevision defaultValue = QTypeRevision())
    {
        const int index = indexOfOwnClassInfo(metaObject, key);
        return (index == -1) ? defaultValue
                             : QTypeRevision::fromEncodedVersion(
                                   QByteArray(metaObject->classInfo(index).value()).toInt());
    }

    inline bool boolClassInfo(const QMetaObject *metaObject, const char *key,
                              bool defaultValue = false)
    {
        const int index = indexOfOwnClassInfo(metaObject, key);
        return (index == -1) ? defaultValue
                             : (QByteArray(metaObject->classInfo(index).value()) == "true");
    }

    inline const char *classElementName(const QMetaObject *metaObject)
    {
        const char *elementName = classInfo(metaObject, "QML.Element");
        if (qstrcmp(elementName, "auto") == 0) {
            const char *strippedClassName = metaObject->className();
            for (const char *c = strippedClassName; *c != '\0'; c++) {
                if (*c == ':')
                    strippedClassName = c + 1;
            }

            return strippedClassName;
        }
        if (qstrcmp(elementName, "anonymous") == 0)
            return nullptr;

        if (!elementName) {
            qWarning().nospace() << "Missing QML.Element class info \"" << elementName << "\""
                                 << " for " << metaObject->className();
        }

        return elementName;
    }

    template<class T, class = std::void_t<>>
    struct QmlExtended
    {
        using Type = void;
    };

    template<class T>
    struct QmlExtended<T, std::void_t<typename T::QmlExtendedType>>
    {
        using Type = typename T::QmlExtendedType;
    };

    template<class T, class = std::void_t<>>
    struct QmlExtendedNamespace
    {
        static constexpr const QMetaObject *metaObject() { return nullptr; }
    };

    template<class T>
    struct QmlExtendedNamespace<T, std::void_t<decltype(T::qmlExtendedNamespace())>>
    {
        static constexpr const QMetaObject *metaObject() { return T::qmlExtendedNamespace(); }
    };

    template<class T, class = std::void_t<>>
    struct QmlResolved
    {
        using Type = T;
    };

    template<class T>
    struct QmlResolved<T, std::void_t<typename T::QmlForeignType>>
    {
        using Type = typename T::QmlForeignType;
    };

    template<class T, class = std::void_t<>>
    struct QmlSingleton
    {
        static constexpr bool Value = false;
    };

    template<class T>
    struct QmlSingleton<T, std::void_t<typename T::QmlIsSingleton>>
    {
        static constexpr bool Value = bool(T::QmlIsSingleton::yes);
    };

    template<class T, class = std::void_t<>>
    struct QmlSequence
    {
        static constexpr bool Value = false;
    };

    template<class T>
    struct QmlSequence<T, std::void_t<typename T::QmlIsSequence>>
    {
        Q_STATIC_ASSERT((std::is_same_v<typename T::QmlSequenceValueType,
                                        typename QmlResolved<T>::Type::value_type>));
        static constexpr bool Value = bool(T::QmlIsSequence::yes);
    };

    template<class T, class = std::void_t<>>
    struct QmlInterface
    {
        static constexpr bool Value = false;
    };

    template<class T>
    struct QmlInterface<T, std::void_t<typename T::QmlIsInterface>>
    {
        static constexpr bool Value = bool(T::QmlIsInterface::yes);
    };

    template<class T, typename = std::void_t<>>
    struct StaticMetaObject
    {
        static const QMetaObject *staticMetaObject() { return nullptr; }
    };

    template<class T>
    struct StaticMetaObject<T, std::void_t<decltype(T::staticMetaObject)>>
    {
        static const QMetaObject *staticMetaObject() { return &T::staticMetaObject; }
    };

    template<class T>
    struct QmlMetaType
    {
        static QMetaType self()
        {
            if constexpr (std::is_base_of_v<QObject, T>)
                return QMetaType::fromType<T*>();
            else
                return QMetaType::fromType<T>();
        }

        static QMetaType list()
        {
            if constexpr (std::is_base_of_v<QObject, T>)
                return QMetaType::fromType<QQmlListProperty<T>>();
            else
                return QMetaType();
        }
    };

    template<typename T, typename E, typename WrapperT = T>
    void qmlRegisterSingletonAndRevisions(const char *uri, int versionMajor,
                                          const QMetaObject *classInfoMetaObject,
                                          QVector<int> *qmlTypeIds, const QMetaObject *extension)
    {
        RegisterSingletonTypeAndRevisions api = {
            0,

            uri,
            QTypeRevision::fromMajorVersion(versionMajor),

            Constructors<T, WrapperT>::createSingletonInstance,

            StaticMetaObject<T>::staticMetaObject(),
            classInfoMetaObject,

            QmlMetaType<T>::self(),

            ExtendedType<E>::createParent,
            extension ? extension : ExtendedType<E>::staticMetaObject(),

            qmlTypeIds
        };

        qmlregister(SingletonAndRevisionsRegistration, &api);
    }

    template<typename T, typename E>
    void qmlRegisterTypeAndRevisions(const char *uri, int versionMajor,
                                     const QMetaObject *classInfoMetaObject,
                                     QVector<int> *qmlTypeIds, const QMetaObject *extension)
    {
        RegisterTypeAndRevisions type = {
            0,
            QmlMetaType<T>::self(),
            QmlMetaType<T>::list(),
            int(sizeof(T)),
            Constructors<T>::createInto, nullptr,
            ValueType<T, E>::create,

            uri,
            QTypeRevision::fromMajorVersion(versionMajor),

            StaticMetaObject<T>::staticMetaObject(),
            classInfoMetaObject,

            attachedPropertiesFunc<T>(),
            attachedPropertiesMetaObject<T>(),

            StaticCastSelector<T, QQmlParserStatus>::cast(),
            StaticCastSelector<T, QQmlPropertyValueSource>::cast(),
            StaticCastSelector<T, QQmlPropertyValueInterceptor>::cast(),

            ExtendedType<E>::createParent,
            extension ? extension : ExtendedType<E>::staticMetaObject(),

            &qmlCreateCustomParser<T>,
            qmlTypeIds
        };

        qmlregister(TypeAndRevisionsRegistration, &type);
    }

    template<typename T>
    void qmlRegisterSequenceAndRevisions(const char *uri, int versionMajor,
                                         const QMetaObject *classInfoMetaObject,
                                         QVector<int> *qmlTypeIds)
    {
        RegisterSequentialContainerAndRevisions type = {
            0,
            uri,
            QTypeRevision::fromMajorVersion(versionMajor),
            classInfoMetaObject,
            QMetaType::fromType<T>(),
            QMetaSequence::fromContainer<T>(),
            qmlTypeIds
        };

        qmlregister(SequentialContainerAndRevisionsRegistration, &type);
    }

    template<>
    void Q_QML_EXPORT qmlRegisterTypeAndRevisions<QQmlTypeNotAvailable, void>(
            const char *uri, int versionMajor, const QMetaObject *classInfoMetaObject,
            QVector<int> *qmlTypeIds, const QMetaObject *);

    constexpr QtPrivate::QMetaTypeInterface metaTypeForNamespace(
            const QtPrivate::QMetaTypeInterface::MetaObjectFn &metaObjectFunction, const char *name)
    {
        return {
            /*.revision=*/ 0,
            /*.alignment=*/ 0,
            /*.size=*/ 0,
            /*.flags=*/ 0,
            /*.typeId=*/ {},
            /*.metaObject=*/ metaObjectFunction,
            /*.name=*/ name,
            /*.defaultCtr=*/ nullptr,
            /*.copyCtr=*/ nullptr,
            /*.moveCtr=*/ nullptr,
            /*.dtor=*/ nullptr,
            /*.equals*/ nullptr,
            /*.lessThan*/ nullptr,
            /*.debugStream=*/ nullptr,
            /*.dataStreamOut=*/ nullptr,
            /*.dataStreamIn=*/ nullptr,
            /*.legacyRegisterOp=*/ nullptr
        };
    }

} // namespace QQmlPrivate

QT_END_NAMESPACE

#endif // QQMLPRIVATE_H
