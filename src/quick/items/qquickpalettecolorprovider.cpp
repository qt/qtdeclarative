/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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
******************************************************************************/
#include "qquickpalettecolorprovider_p.h"

#include <QtQuick/private/qquickabstractpaletteprovider_p.h>

QT_BEGIN_NAMESPACE

static bool notEq(const QPalette &p1, const QPalette &p2)
{
    return p1.resolveMask() != p2.resolveMask() || p1 != p2;
}

static QPalette::ColorGroup adjustCg(QPalette::ColorGroup group)
{
    return group == QPalette::All ? QPalette::Active : group;
}

class DefaultPalettesProvider : public QQuickAbstractPaletteProvider
{
public:
    QPalette defaultPalette() const override { static QPalette p; return p; }
    QPalette parentPalette() const override  { return defaultPalette();     }
};

static std::default_delete<const QQuickAbstractPaletteProvider> defaultDeleter() { return {}; }

QQuickPaletteColorProvider::QQuickPaletteColorProvider()
    : m_paletteProvider(ProviderPtr(new DefaultPalettesProvider, defaultDeleter()))
{
}

const QColor &QQuickPaletteColorProvider::color(QPalette::ColorGroup group, QPalette::ColorRole role) const
{
    return m_resolvedPalette.color(adjustCg(group), role);
}

bool QQuickPaletteColorProvider::setColor(QPalette::ColorGroup g, QPalette::ColorRole r, QColor c)
{
    m_requestedPalette.value() = m_resolvedPalette;
    m_requestedPalette->setColor(g, r, c);

    return inheritPalette(paletteProvider()->parentPalette());
}

bool QQuickPaletteColorProvider::resetColor(QPalette::ColorGroup group, QPalette::ColorRole role)
{
    const auto &defaultPalette = paletteProvider()->defaultPalette() ;
    const auto &defaultColor = defaultPalette.color(adjustCg(group), role);

    return setColor(group, role, defaultColor);
}

bool QQuickPaletteColorProvider::fromQPalette(QPalette p)
{
    m_requestedPalette.value() = std::move(p);
    return inheritPalette(paletteProvider()->parentPalette());
}

QPalette QQuickPaletteColorProvider::palette() const
{
    return m_resolvedPalette;
}

const QQuickAbstractPaletteProvider *QQuickPaletteColorProvider::paletteProvider() const
{
    Q_ASSERT(m_paletteProvider);
    return m_paletteProvider.get();
}

void QQuickPaletteColorProvider::setPaletteProvider(const QQuickAbstractPaletteProvider *paletteProvider)
{
    static const auto emptyDeleter = [](auto &&){};
    m_paletteProvider = ProviderPtr(paletteProvider, emptyDeleter);
}

bool QQuickPaletteColorProvider::copyColorGroup(QPalette::ColorGroup cg,
                                                const QQuickPaletteColorProvider &p)
{
    m_requestedPalette.value() = m_resolvedPalette;

    auto srcPalette = p.palette();
    for (int roleIndex = QPalette::WindowText; roleIndex < QPalette::NColorRoles; ++roleIndex) {
        const auto cr = QPalette::ColorRole(roleIndex);
        if (srcPalette.isBrushSet(cg, cr)) {
            m_requestedPalette->setBrush(cg, cr, srcPalette.brush(cg, cr));
        }
    }

    return inheritPalette(paletteProvider()->parentPalette());
}

bool QQuickPaletteColorProvider::reset()
{
    return fromQPalette(QPalette());
}

bool QQuickPaletteColorProvider::inheritPalette(const QPalette &p)
{
    auto inheritedMask = m_requestedPalette.isAllocated() ? m_requestedPalette->resolveMask() | p.resolveMask() : p.resolveMask();
    QPalette parentPalette = m_requestedPalette.isAllocated() ? m_requestedPalette->resolve(p) : p;
    parentPalette.setResolveMask(inheritedMask);

    auto tmpResolvedPalette = parentPalette.resolve(paletteProvider()->defaultPalette());
    tmpResolvedPalette.setResolveMask(tmpResolvedPalette.resolveMask() | inheritedMask);

    bool changed = notEq(tmpResolvedPalette, m_resolvedPalette);
    if (changed) {
        std::swap(tmpResolvedPalette, m_resolvedPalette);
    }

    return changed;
}

QT_END_NAMESPACE
