/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "qqmljsutils_p.h"

#include <algorithm>

using namespace Qt::StringLiterals;

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
