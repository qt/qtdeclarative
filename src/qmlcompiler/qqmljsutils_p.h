// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLJSUTILS_P_H
#define QQMLJSUTILS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <private/qtqmlcompilerexports_p.h>

#include "qqmljslogger_p.h"
#include "qqmljsscope_p.h"
#include "qqmljsmetatypes_p.h"

#include <QtCore/qstack.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringview.h>
#include <QtCore/qstringbuilder.h>
#include <private/qduplicatetracker_p.h>

#include <optional>
#include <functional>
#include <type_traits>

QT_BEGIN_NAMESPACE

namespace detail {
/*! \internal

    Utility method that returns proper value according to the type To. This
    version returns From.
*/
template<typename To, typename From, typename std::enable_if_t<!std::is_pointer_v<To>, int> = 0>
static auto getQQmlJSScopeFromSmartPtr(const From &p) -> From
{
    static_assert(!std::is_pointer_v<From>, "From has to be a smart pointer holding QQmlJSScope");
    return p;
}

/*! \internal

    Utility method that returns proper value according to the type To. This
    version returns From::get(), which is a raw pointer. The returned type
    is not necessarily equal to To (e.g. To might be `QQmlJSScope *` while
    returned is `const QQmlJSScope *`).
*/
template<typename To, typename From, typename std::enable_if_t<std::is_pointer_v<To>, int> = 0>
static auto getQQmlJSScopeFromSmartPtr(const From &p) -> decltype(p.get())
{
    static_assert(!std::is_pointer_v<From>, "From has to be a smart pointer holding QQmlJSScope");
    return p.get();
}
}

class QQmlJSTypeResolver;
class QQmlJSScopesById;
struct Q_QMLCOMPILER_PRIVATE_EXPORT QQmlJSUtils
{
    /*! \internal
        Returns escaped version of \a s. This function is mostly useful for code
        generators.
    */
    static QString escapeString(QString s)
    {
        using namespace Qt::StringLiterals;
        return s.replace(u'\\', u"\\\\"_s).replace(u'"', u"\\\""_s).replace(u'\n', u"\\n"_s);
    }

    /*! \internal
        Returns \a s wrapped into a literal macro specified by \a ctor. By
        default, returns a QStringLiteral-wrapped literal. This function is
        mostly useful for code generators.

        \note This function escapes \a s before wrapping it.
    */
    static QString toLiteral(const QString &s, QStringView ctor = u"QStringLiteral")
    {
        return ctor % u"(\"" % escapeString(s) % u"\")";
    }

    /*! \internal
        Returns \a type string conditionally wrapped into \c{const} and \c{&}.
        This function is mostly useful for code generators.
    */
    static QString constRefify(QString type)
    {
        if (!type.endsWith(u'*'))
            type = u"const " % type % u"&";
        return type;
    }

    /*! \internal
        Returns a signal name from \a handlerName string.
    */
    static std::optional<QString> signalName(QStringView handlerName)
    {
        if (handlerName.startsWith(u"on") && handlerName.size() > 2) {
            QString signal = handlerName.mid(2).toString();
            for (int i = 0; i < signal.length(); ++i) {
                QChar &ch = signal[i];
                if (ch.isLower())
                    return {};
                if (ch.isUpper()) {
                    ch = ch.toLower();
                    return signal;
                }
            }
        }
        return {};
    }

    static std::optional<QQmlJSMetaProperty>
    changeHandlerProperty(const QQmlJSScope::ConstPtr &scope, QStringView signalName)
    {
        if (!signalName.endsWith(QLatin1String("Changed")))
            return {};
        constexpr int length = int(sizeof("Changed") / sizeof(char)) - 1;
        signalName.chop(length);
        auto p = scope->property(signalName.toString());
        const bool isBindable = !p.bindable().isEmpty();
        const bool canNotify = !p.notify().isEmpty();
        if (p.isValid() && (isBindable || canNotify))
            return p;
        return {};
    }

    static bool hasCompositeBase(const QQmlJSScope::ConstPtr &scope)
    {
        if (!scope)
            return false;
        const auto base = scope->baseType();
        if (!base)
            return false;
        return base->isComposite() && base->scopeType() == QQmlJSScope::QMLScope;
    }

    enum ResolvedAliasTarget {
        AliasTarget_Invalid,
        AliasTarget_Property,
        AliasTarget_Object,
    };
    struct ResolvedAlias
    {
        QQmlJSMetaProperty property;
        QQmlJSScope::ConstPtr owner;
        ResolvedAliasTarget kind = ResolvedAliasTarget::AliasTarget_Invalid;
    };
    struct AliasResolutionVisitor
    {
        std::function<void()> reset = []() {};
        std::function<void(const QQmlJSScope::ConstPtr &)> processResolvedId =
                [](const QQmlJSScope::ConstPtr &) {};
        std::function<void(const QQmlJSMetaProperty &, const QQmlJSScope::ConstPtr &)>
                processResolvedProperty =
                        [](const QQmlJSMetaProperty &, const QQmlJSScope::ConstPtr &) {};
    };
    static ResolvedAlias resolveAlias(const QQmlJSTypeResolver *typeResolver,
                                      const QQmlJSMetaProperty &property,
                                      const QQmlJSScope::ConstPtr &owner,
                                      const AliasResolutionVisitor &visitor);
    static ResolvedAlias resolveAlias(const QQmlJSScopesById &idScopes,
                                      const QQmlJSMetaProperty &property,
                                      const QQmlJSScope::ConstPtr &owner,
                                      const AliasResolutionVisitor &visitor);

    template<typename QQmlJSScopePtr, typename Action>
    static bool searchBaseAndExtensionTypes(QQmlJSScopePtr type, const Action &check)
    {
        if (!type)
            return false;

        using namespace detail;

        // NB: among other things, getQQmlJSScopeFromSmartPtr() also resolves const
        // vs non-const pointer issue, so use it's return value as the type
        using T = decltype(getQQmlJSScopeFromSmartPtr<QQmlJSScopePtr>(
                std::declval<QQmlJSScope::ConstPtr>()));

        const auto checkWrapper = [&](const auto &scope, QQmlJSScope::ExtensionKind mode) {
            if constexpr (std::is_invocable<Action, decltype(scope),
                                            QQmlJSScope::ExtensionKind>::value) {
                return check(scope, mode);
            } else {
                static_assert(std::is_invocable<Action, decltype(scope)>::value,
                              "Inferred type Action has unexpected arguments");
                Q_UNUSED(mode);
                return check(scope);
            }
        };

        const bool isValueType = (type->accessSemantics() == QQmlJSScope::AccessSemantics::Value);

        QDuplicateTracker<T> seen;
        for (T scope = type; scope && !seen.hasSeen(scope);
             scope = getQQmlJSScopeFromSmartPtr<QQmlJSScopePtr>(scope->baseType())) {
            QDuplicateTracker<T> seenExtensions;
            // Extensions override the types they extend. However, usually base
            // types of extensions are ignored. The unusual cases are when we
            // have a value type or when we have the QObject type, in which case
            // we also study the extension's base type hierarchy.
            const bool isQObject = scope->internalName() == QLatin1String("QObject");
            auto [extensionPtr, extensionKind] = scope->extensionType();
            auto extension = getQQmlJSScopeFromSmartPtr<QQmlJSScopePtr>(extensionPtr);
            do {
                if (!extension || seenExtensions.hasSeen(extension))
                    break;

                if (checkWrapper(extension, extensionKind))
                    return true;
                extension = getQQmlJSScopeFromSmartPtr<QQmlJSScopePtr>(extension->baseType());
            } while (isValueType || isQObject);

            if (checkWrapper(scope, QQmlJSScope::NotExtension))
                return true;
        }

        return false;
    }

    template<typename Action>
    static void traverseFollowingQmlIrObjectStructure(const QQmlJSScope::Ptr &root, Action act)
    {
        // We *have* to perform DFS here: QmlIR::Object entries within the
        // QmlIR::Document are stored in the order they appear during AST traversal
        // (which does DFS)
        QStack<QQmlJSScope::Ptr> stack;
        stack.push(root);

        while (!stack.isEmpty()) {
            QQmlJSScope::Ptr current = stack.pop();

            act(current);

            auto children = current->childScopes();
            // arrays are special: they are reverse-processed in QmlIRBuilder
            if (!current->isArrayScope())
                std::reverse(children.begin(), children.end()); // left-to-right DFS
            stack.append(std::move(children));
        }
    }

    static std::optional<FixSuggestion> didYouMean(const QString &userInput,
                                                   QStringList candidates,
                                                   QQmlJS::SourceLocation location);
};

QT_END_NAMESPACE

#endif // QQMLJSUTILS_P_H
