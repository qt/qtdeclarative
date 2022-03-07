// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
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
    ensureRequestedPalette();
    m_requestedPalette->setColor(g, r, c);

    return updateInheritedPalette();
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
    return updateInheritedPalette();
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
    ensureRequestedPalette();

    auto srcPalette = p.palette();
    for (int roleIndex = QPalette::WindowText; roleIndex < QPalette::NColorRoles; ++roleIndex) {
        const auto cr = QPalette::ColorRole(roleIndex);
        if (srcPalette.isBrushSet(cg, cr)) {
            m_requestedPalette->setBrush(cg, cr, srcPalette.brush(cg, cr));
        }
    }

    return updateInheritedPalette();
}

bool QQuickPaletteColorProvider::reset()
{
    return fromQPalette(QPalette());
}

/*! \internal
    Merge the given \a palette with the existing requested palette, remember
    that it is the inherited palette (in case updateInheritedPalette() is
    called later), and update the stored palette (to be returned from
    \l palette()) if the result is different. Returns whether the stored
    palette got changed.
*/
bool QQuickPaletteColorProvider::inheritPalette(const QPalette &palette)
{
    m_lastInheritedPalette.value() = palette;
    return doInheritPalette(palette);
}

/*! \internal
    Merge the given \a palette with the existing requested palette, and update
    the stored palette (to be returned from \l palette()) if the result is
    different. Returns whether the stored palette got changed.
*/
bool QQuickPaletteColorProvider::doInheritPalette(const QPalette &palette)
{
    auto inheritedMask = m_requestedPalette.isAllocated() ? m_requestedPalette->resolveMask() | palette.resolveMask()
                                                          : palette.resolveMask();
    QPalette parentPalette = m_requestedPalette.isAllocated() ? m_requestedPalette->resolve(palette) : palette;
    parentPalette.setResolveMask(inheritedMask);

    auto tmpResolvedPalette = parentPalette.resolve(paletteProvider()->defaultPalette());
    tmpResolvedPalette.setResolveMask(tmpResolvedPalette.resolveMask() | inheritedMask);

    bool changed = notEq(tmpResolvedPalette, m_resolvedPalette);
    if (changed)
        std::swap(tmpResolvedPalette, m_resolvedPalette);

    return changed;
}

/*! \internal
    Update the stored palette (to be returned from \l palette()) from the
    parent palette. Returns whether the stored palette got changed.
*/
bool QQuickPaletteColorProvider::updateInheritedPalette()
{
    // Use last inherited palette as parentPalette's fallbackPalette: it's useful when parentPalette doesn't exist.
    const QPalette &p = m_lastInheritedPalette.isAllocated() ? m_lastInheritedPalette.value()
                                                             : paletteProvider()->defaultPalette();
    return doInheritPalette(paletteProvider()->parentPalette(p));
}

void QQuickPaletteColorProvider::ensureRequestedPalette()
{
    if (m_requestedPalette.isAllocated())
        return;

    m_requestedPalette.value() = QPalette();
}

QT_END_NAMESPACE
