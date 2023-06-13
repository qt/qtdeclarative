// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickpalette_p.h"

#include <QtQuick/private/qquickpalettecolorprovider_p.h>

QT_BEGIN_NAMESPACE

static constexpr bool is_valid(QPalette::ColorGroup cg) noexcept
{
    // use a switch to enable "unhandled enum" warnings:
    switch (cg) {
    case QPalette::Active:
    case QPalette::Disabled:
    case QPalette::Inactive:
        return true;
    case QPalette::NColorGroups:
    case QPalette::Current:
    case QPalette::All:
        return false;
    }

    // GCC 8.x does not tread __builtin_unreachable() as constexpr
#if defined(Q_CC_INTEL) || defined(Q_CC_CLANG) || (defined(Q_CC_GNU) && Q_CC_GNU >= 900)
    // NOLINTNEXTLINE(qt-use-unreachable-return): Triggers on Clang, breaking GCC 8
    Q_UNREACHABLE();
#endif
    return false;
}

/*!
    \internal

    \class QQuickPalette
    \brief Contains color groups for each QML item state.
    \inmodule QtQuick
    \since 6.0

    This class is the wrapper around QPalette.

    \sa QQuickColorGroup, QQuickAbstractPaletteProvider, QPalette
 */

/*!
    \qmltype Palette
    \instantiates QQuickPalette
    \inherits QQuickColorGroup
    \inqmlmodule QtQuick
    \ingroup qtquick-visual
    \brief Contains color groups for each QML item state.

    A palette consists of three color groups: \c active, \c disabled, and \c inactive.
    The \c active color group is the default group: its colors are used for other groups
    if colors of these groups aren't explicitly specified.

    In the following example, color is applied for all color groups:
    \code
    ApplicationWindow {
        palette.buttonText: "salmon"

        ColumnLayout {
            Button {
                text: qsTr("Disabled button")
                enabled: false
            }

            Button {
                text: qsTr("Enabled button")
            }
        }
    }
    \endcode
    It means that text color will be the same for both buttons.

    In the following example, colors will be different for enabled and disabled states:
    \code
    ApplicationWindow {
        palette.buttonText: "salmon"
        palette.disabled.buttonText: "lavender"

        ColumnLayout {
            Button {
                text: qsTr("Disabled button")
                enabled: false
            }

            Button {
                text: qsTr("Enabled button")
            }
        }
    }
    \endcode

    It is also possible to specify colors like this:

    \snippet qtquickcontrols-custom-palette-buttons.qml palette

    This approach is especially convenient when you need to specify a whole
    palette with all color groups; but as with the other cases above, the
    colors that are not specified are initialized from SystemPalette, or
    potentially the \l {Styling Qt Quick Controls}{Qt Quick Controls style},
    if one is in use.

    \note Some Controls styles use some palette colors, but many styles use
    independent colors.

    \sa Window::palette, Item::palette, Popup::palette, SystemPalette
*/

/*!
    \qmlproperty ColorGroup QtQuick::Palette::active

    The Active group is used for windows that are in focus.

    \sa QPalette::Active
*/

/*!
    \qmlproperty ColorGroup QtQuick::Palette::inactive

    The Inactive group is used for windows that have no keyboard focus.

    \sa QPalette::Inactive
*/

/*!
    \qmlproperty ColorGroup QtQuick::Palette::disabled

    The Disabled group is used for elements that are disabled for some reason.

    \sa QPalette::Disabled
*/

QQuickPalette::QQuickPalette(QObject *parent)
    : QQuickColorGroup(parent)
    , m_currentGroup(defaultCurrentGroup())
{
}

QQuickColorGroup *QQuickPalette::active() const
{
    return colorGroup(QPalette::Active);
}

QQuickColorGroup *QQuickPalette::inactive() const
{
    return colorGroup(QPalette::Inactive);
}

QQuickColorGroup *QQuickPalette::disabled() const
{
    return colorGroup(QPalette::Disabled);
}

/*!
    \internal

    Returns the palette's current color group.
    The default value is Active.
 */
QPalette::ColorGroup QQuickPalette::currentColorGroup() const
{
    return m_currentGroup;
}

/*!
    \internal

    Sets \a currentGroup for this palette.

    The current color group is used when accessing colors of this palette.
    For example, if color group is Disabled, color accessors will be
    returning colors form the respective group.
    \code
    QQuickPalette palette;

    palette.setAlternateBase(Qt::green);
    palette.disabled()->setAlternateBase(Qt::red);

    auto color = palette.alternateBase(); // Qt::green

    palette.setCurrentGroup(QPalette::Disabled);
    color = palette.alternateBase(); // Qt::red
    \endcode

    Emits QColorGroup::changed().
 */
void QQuickPalette::setCurrentGroup(QPalette::ColorGroup currentGroup)
{
    if (m_currentGroup != currentGroup) {
        m_currentGroup = currentGroup;
        Q_EMIT changed();
    }
}

void QQuickPalette::fromQPalette(QPalette palette)
{
    if (colorProvider().fromQPalette(std::move(palette))) {
        Q_EMIT changed();
    }
}

QPalette QQuickPalette::toQPalette() const
{
    return colorProvider().palette();
}

const QQuickAbstractPaletteProvider *QQuickPalette::paletteProvider() const
{
    return colorProvider().paletteProvider();
}

void QQuickPalette::setPaletteProvider(const QQuickAbstractPaletteProvider *paletteProvider)
{
    colorProvider().setPaletteProvider(paletteProvider);
}

void QQuickPalette::reset()
{
    if (colorProvider().reset()) {
        Q_EMIT changed();
    }
}

void QQuickPalette::inheritPalette(const QPalette &palette)
{
    if (colorProvider().inheritPalette(palette)) {
        Q_EMIT changed();
    }
}

void QQuickPalette::setActive(QQuickColorGroup *active)
{
    setColorGroup(QPalette::Active, active, &QQuickPalette::activeChanged);
}

void QQuickPalette::setInactive(QQuickColorGroup *inactive)
{
    setColorGroup(QPalette::Inactive, inactive, &QQuickPalette::inactiveChanged);
}

void QQuickPalette::setDisabled(QQuickColorGroup *disabled)
{
    setColorGroup(QPalette::Disabled, disabled, &QQuickPalette::disabledChanged);
}


void QQuickPalette::setColorGroup(QPalette::ColorGroup groupTag,
                                  const QQuickColorGroup::GroupPtr &group,
                                  void (QQuickPalette::*notifier)())
{
    if (isValidColorGroup(groupTag, group)) {
        if (colorProvider().copyColorGroup(groupTag, group->colorProvider())) {
            Q_EMIT (this->*notifier)();
            Q_EMIT changed();
        }
    }
}

QQuickColorGroup::GroupPtr QQuickPalette::colorGroup(QPalette::ColorGroup groupTag) const
{
    if (auto group = findColorGroup(groupTag)) {
        return group;
    }

    auto group = QQuickColorGroup::createWithParent(*const_cast<QQuickPalette*>(this));
    const_cast<QQuickPalette*>(this)->registerColorGroup(group, groupTag);
    return group;
}

QQuickColorGroup::GroupPtr QQuickPalette::findColorGroup(QPalette::ColorGroup groupTag) const
{
    Q_ASSERT(is_valid(groupTag));
    return m_colorGroups[groupTag];
}

void QQuickPalette::registerColorGroup(QQuickColorGroup *group, QPalette::ColorGroup groupTag)
{
    Q_ASSERT(is_valid(groupTag));
    auto &g = m_colorGroups[groupTag];
    if (g) {
        Q_ASSERT(g != group);
        g->deleteLater();
    }
    g = group;

    group->setGroupTag(groupTag);

    QQuickColorGroup::connect(group, &QQuickColorGroup::changed, this, &QQuickPalette::changed);
}

bool QQuickPalette::isValidColorGroup(QPalette::ColorGroup groupTag,
                                      const QQuickColorGroup::GroupPtr &colorGroup) const
{
    if (!colorGroup) {
        qWarning("Color group cannot be null.");
        return false;
    }

    if (!colorGroup->parent()) {
        qWarning("Color group should have a parent.");
        return false;
    }

    if (colorGroup->parent() && !qobject_cast<QQuickPalette*>(colorGroup->parent())) {
        qWarning("Color group should be a part of QQuickPalette.");
        return false;
    }

    if (groupTag == defaultGroupTag()) {
        qWarning("Register %i color group is not allowed."
                 " QQuickPalette is %i color group itself.", groupTag, groupTag);
        return false;
    }

    if (findColorGroup(groupTag) == colorGroup) {
        qWarning("The color group is already a part of the current palette.");
        return false;
    }

    return true;
}

QT_END_NAMESPACE

#include "moc_qquickpalette_p.cpp"
