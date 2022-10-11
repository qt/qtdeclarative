// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsutils_p.h"
#include "qqmljstyperesolver_p.h"
#include "qqmljsscopesbyid_p.h"

#include <algorithm>

using namespace Qt::StringLiterals;

/*! \internal

    Fully resolves alias \a property and returns the information about the
    origin, which is not an alias.
*/
template<typename ScopeForId>
static QQmlJSUtils::ResolvedAlias
resolveAlias(ScopeForId scopeForId, const QQmlJSMetaProperty &property,
             const QQmlJSScope::ConstPtr &owner, const QQmlJSUtils::AliasResolutionVisitor &visitor)
{
    Q_ASSERT(property.isAlias());
    Q_ASSERT(owner);

    QQmlJSUtils::ResolvedAlias result {};
    result.owner = owner;

    // TODO: one could optimize the generated alias code for aliases pointing to aliases
    //  e.g., if idA.myAlias -> idB.myAlias2 -> idC.myProp, then one could directly generate
    //  idA.myProp as pointing to idC.myProp.
    //  This gets complicated when idB.myAlias is in a different Component than where the
    //  idA.myAlias is defined: scopeForId currently only contains the ids of the current
    //  component and alias resolution on the ids of a different component fails then.
    if (QQmlJSMetaProperty nextProperty = property; nextProperty.isAlias()) {
        QQmlJSScope::ConstPtr resultOwner = result.owner;
        result = QQmlJSUtils::ResolvedAlias {};

        visitor.reset();

        auto aliasExprBits = nextProperty.aliasExpression().split(u'.');
        // do not crash on invalid aliasexprbits when accessing aliasExprBits[0]
        if (aliasExprBits.size() < 1)
            return {};

        // resolve id first:
        resultOwner = scopeForId(aliasExprBits[0], resultOwner);
        if (!resultOwner)
            return {};

        visitor.processResolvedId(resultOwner);

        aliasExprBits.removeFirst(); // Note: for simplicity, remove the <id>
        result.owner = resultOwner;
        result.kind = QQmlJSUtils::AliasTarget_Object;

        for (const QString &bit : qAsConst(aliasExprBits)) {
            nextProperty = resultOwner->property(bit);
            if (!nextProperty.isValid())
                return {};

            visitor.processResolvedProperty(nextProperty, resultOwner);

            result.property = nextProperty;
            result.owner = resultOwner;
            result.kind = QQmlJSUtils::AliasTarget_Property;

            resultOwner = nextProperty.type();
        }
    }

    return result;
}

QQmlJSUtils::ResolvedAlias QQmlJSUtils::resolveAlias(const QQmlJSTypeResolver *typeResolver,
                                                     const QQmlJSMetaProperty &property,
                                                     const QQmlJSScope::ConstPtr &owner,
                                                     const AliasResolutionVisitor &visitor)
{
    return ::resolveAlias(
            [&](const QString &id, const QQmlJSScope::ConstPtr &referrer) {
                return typeResolver->scopeForId(id, referrer);
            },
            property, owner, visitor);
}

QQmlJSUtils::ResolvedAlias QQmlJSUtils::resolveAlias(const QQmlJSScopesById &idScopes,
                                                     const QQmlJSMetaProperty &property,
                                                     const QQmlJSScope::ConstPtr &owner,
                                                     const AliasResolutionVisitor &visitor)
{
    return ::resolveAlias(
            [&](const QString &id, const QQmlJSScope::ConstPtr &referrer) {
                return idScopes.scope(id, referrer);
            },
            property, owner, visitor);
}

std::optional<FixSuggestion> QQmlJSUtils::didYouMean(const QString &userInput,
                                                     QStringList candidates,
                                                     QQmlJS::SourceLocation location)
{
    QString shortestDistanceWord;
    int shortestDistance = userInput.length();

    // Most of the time the candidates are keys() from QHash, which means that
    // running this function in the seemingly same setup might yield different
    // best cadidate (e.g. imagine a typo 'thing' with candidates 'thingA' vs
    // 'thingB'). This is especially flaky in e.g. test environment where the
    // results may differ (even when the global hash seed is fixed!) when
    // running one test vs the whole test suite (recall platform-dependent
    // QSKIPs). There could be user-visible side effects as well, so just sort
    // the candidates to guarantee consistent results
    std::sort(candidates.begin(), candidates.end());

    for (const QString &candidate : candidates) {
        /*
         * Calculate the distance between the userInput and candidate using Damerau–Levenshtein
         * Roughly based on
         * https://en.wikipedia.org/wiki/Levenshtein_distance#Iterative_with_two_matrix_rows.
         */
        QList<int> v0(candidate.length() + 1);
        QList<int> v1(candidate.length() + 1);

        std::iota(v0.begin(), v0.end(), 0);

        for (qsizetype i = 0; i < userInput.length(); i++) {
            v1[0] = i + 1;
            for (qsizetype j = 0; j < candidate.length(); j++) {
                int deletionCost = v0[j + 1] + 1;
                int insertionCost = v1[j] + 1;
                int substitutionCost = userInput[i] == candidate[j] ? v0[j] : v0[j] + 1;
                v1[j + 1] = std::min({ deletionCost, insertionCost, substitutionCost });
            }
            std::swap(v0, v1);
        }

        int distance = v0[candidate.length()];
        if (distance < shortestDistance) {
            shortestDistanceWord = candidate;
            shortestDistance = distance;
        }
    }

    if (shortestDistance
        < std::min(std::max(userInput.length() / 2, qsizetype(3)), userInput.length())) {
        return FixSuggestion { { FixSuggestion::Fix {
                u"Did you mean \"%1\"?"_s.arg(shortestDistanceWord), location,
                shortestDistanceWord } } };
    } else {
        return {};
    }
}
