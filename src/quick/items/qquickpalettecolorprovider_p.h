/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QQUICKPALETTECOLORPROVIDER_P_H
#define QQUICKPALETTECOLORPROVIDER_P_H

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

#include <QtGui/QPalette>

#include <QtQuick/qtquickglobal.h>

#include <QtQuick/private/qtquickglobal_p.h>
#include <private/qlazilyallocated_p.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractPaletteProvider;

class Q_QUICK_PRIVATE_EXPORT QQuickPaletteColorProvider
    : public std::enable_shared_from_this<QQuickPaletteColorProvider>
{
public:
    QQuickPaletteColorProvider();

    const QColor &color(QPalette::ColorGroup group, QPalette::ColorRole role) const;
    bool setColor(QPalette::ColorGroup group, QPalette::ColorRole role, QColor color);
    bool resetColor(QPalette::ColorGroup group, QPalette::ColorRole role);

    bool fromQPalette(QPalette p);
    QPalette palette() const;

    const QQuickAbstractPaletteProvider *paletteProvider() const;
    void setPaletteProvider(const QQuickAbstractPaletteProvider *paletteProvider);

    bool copyColorGroup(QPalette::ColorGroup cg, const QQuickPaletteColorProvider &p);

    bool reset();

    bool inheritPalette(const QPalette &p);

private:
    QPalette m_resolvedPalette;
    QLazilyAllocated<QPalette> m_requestedPalette;

    using Deleter = std::function<void(const QQuickAbstractPaletteProvider*)>;
    using ProviderPtr = std::unique_ptr<const QQuickAbstractPaletteProvider, Deleter>;
    ProviderPtr m_paletteProvider;
};

QT_END_NAMESPACE

#endif // QQUICKPALETTECOLORPROVIDER_P_H
