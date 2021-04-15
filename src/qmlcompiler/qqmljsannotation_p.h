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

#ifndef QQMLJSANNOTATION_P_H
#define QQMLJSANNOTATION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <variant>

#include <private/qqmljsast_p.h>
#include <qglobal.h>

QT_BEGIN_NAMESPACE

struct QQQmlJSDeprecation
{
    QString reason;
};

struct QQmlJSAnnotation
{
    using Value = std::variant<QString, double>;

    QString name;
    QHash<QString, Value> bindings;

    bool isDeprecation() const;
    QQQmlJSDeprecation deprecation() const;

    friend bool operator==(const QQmlJSAnnotation &a, const QQmlJSAnnotation &b) {
        return a.name == b.name &&
               a.bindings == b.bindings;
    }

    friend bool operator!=(const QQmlJSAnnotation &a, const QQmlJSAnnotation &b) {
        return !(a == b);
    }

    friend size_t qHash(const QQmlJSAnnotation &annotation, size_t seed = 0)
    {
        QtPrivate::QHashCombine combine;
        seed = combine(seed, annotation.name);

        for (auto it = annotation.bindings.constBegin(); it != annotation.bindings.constEnd(); ++it) {
            QtPrivate::QHashCombine combine;
            size_t h = combine(seed, it.key());
            // use + to keep the result independent of the ordering of the keys

            const auto &var = it.value();

            if (var.index() == std::variant_npos)
                continue;

            if (std::holds_alternative<double>(var))
                seed += combine(h, std::get<double>(var));
            else if (std::holds_alternative<QString>(var))
                seed += combine(h, std::get<QString>(var));
            else
                Q_UNREACHABLE();
        }

        return seed;
    }
};

QT_END_NAMESPACE

#endif // QQMLJSANNOTATION_P_H
