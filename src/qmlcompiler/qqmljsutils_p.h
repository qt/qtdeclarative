/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

#include <QtCore/qstring.h>
#include <QtCore/qstringview.h>
#include <QtCore/qstringbuilder.h>

#include <optional>
#include <functional>

QT_BEGIN_NAMESPACE

class QQmlJSTypeResolver;
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
                                      QQmlJSMetaProperty property, QQmlJSScope::ConstPtr owner,
                                      const AliasResolutionVisitor &visitor);

    static std::optional<FixSuggestion> didYouMean(const QString &userInput,
                                                   QStringList candidates,
                                                   QQmlJS::SourceLocation location);
};

QT_END_NAMESPACE

#endif // QQMLJSUTILS_P_H
