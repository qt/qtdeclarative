// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qquickiconimage_p.h"
#include "qquickmenuitemiconlabel_p.h"
#include "qquickmenuitemiconlabel_p_p.h"
#include "qquickmnemoniclabel_p.h"

#include <QtQuick/private/qquicktext_p.h>
#include <QtQuickTemplates2/private/qquickaction_p.h>
#include <QtQuickTemplates2/private/qquickmenuitem_p.h>

QT_BEGIN_NAMESPACE

QQuickMenuItemIconLabelPrivate::~QQuickMenuItemIconLabelPrivate() = default;

bool QQuickMenuItemIconLabelPrivate::hasShortcut() const
{
#if QT_CONFIG(shortcut)
    // See comment in layout() for why we don't support TextUnderIcon for shortcuts.
    return (display == QQuickIconLabel::TextOnly || display == QQuickIconLabel::TextBesideIcon)
        // Don't store this statically because we want to be able to auto-test it.
        && !QCoreApplication::testAttribute(Qt::AA_DontShowShortcutsInContextMenus)
        && !shortcut().isEmpty();
#else
    return false;
#endif
}

#if QT_CONFIG(shortcut)
QKeySequence QQuickMenuItemIconLabelPrivate::shortcut() const
{
    return menuItem && menuItem->action() ? menuItem->action()->shortcut() : QKeySequence();
}
#endif

bool QQuickMenuItemIconLabelPrivate::createShortcutLabel()
{
    Q_Q(QQuickIconLabel);
    if (shortcutLabel)
        return false;

    shortcutLabel = new QQuickText(q);
    watchChanges(shortcutLabel);
    beginClass(shortcutLabel);
    shortcutLabel->setObjectName(QStringLiteral("shortcutLabel"));
    shortcutLabel->setFont(font);
    shortcutLabel->setColor(color);
    // We don't set elide because it shouldn't.
    shortcutLabel->setVAlign(QQuickText::AlignVCenter);
    shortcutLabel->setHAlign(QQuickText::AlignHCenter);
#if QT_CONFIG(shortcut)
    shortcutLabel->setText(shortcut().toString(QKeySequence::NativeText));
#endif
    if (componentComplete)
        completeComponent(shortcutLabel);
    return true;
}

bool QQuickMenuItemIconLabelPrivate::destroyShortcutLabel()
{
    if (!shortcutLabel)
        return false;

    unwatchChanges(shortcutLabel);
    delete shortcutLabel;
    shortcutLabel = nullptr;
    return true;
}

void QQuickMenuItemIconLabelPrivate::updateOrSyncShortcutLabel()
{
    if (updateShortcutLabel()) {
        if (componentComplete) {
            updateImplicitSize();
            layout();
        }
    } else {
        syncShortcutLabel();
    }
}

// This and layout should be kept in sync with the base class functions.
// I couldn't think of an elegant way to use polymorphism here.
void QQuickMenuItemIconLabelPrivate::updateImplicitSize()
{
    Q_Q(QQuickMenuItemIconLabel);
    const bool showIcon = image && hasIcon();
    const bool showText = label && hasText();
#if QT_CONFIG(shortcut)
    const bool showShortcutText = shortcutLabel && !shortcut().isEmpty();
#else
    const bool showShortcutText = false;
#endif
    const qreal horizontalPadding = leftPadding + rightPadding;
    const qreal verticalPadding = topPadding + bottomPadding;
    const qreal iconImplicitWidth = showIcon ? image->implicitWidth() : 0;
    const qreal iconImplicitHeight = showIcon ? image->implicitHeight() : 0;
    const qreal textImplicitWidth = showText ? label->implicitWidth() : 0;
    const qreal textImplicitHeight = showText ? label->implicitHeight() : 0;
    const qreal shortcutTextImplicitWidth = showShortcutText ? shortcutLabel->implicitWidth() : 0;
    const qreal shortcutTextImplicitHeight = showShortcutText ? shortcutLabel->implicitHeight() : 0;
    qreal effectiveSpacing = showText && showIcon && image->implicitWidth() > 0 ? spacing : 0;
    if (showShortcutText && shortcutLabel->implicitWidth() > 0)
        effectiveSpacing += spacing;
    const qreal implicitWidth = display == QQuickIconLabel::TextBesideIcon
        ? iconImplicitWidth + textImplicitWidth + shortcutTextImplicitHeight + effectiveSpacing
        : qMax(qMax(iconImplicitWidth, textImplicitWidth), shortcutTextImplicitWidth);
    const qreal implicitHeight = display == QQuickIconLabel::TextUnderIcon
        ? iconImplicitHeight + textImplicitHeight + shortcutTextImplicitHeight + effectiveSpacing
        : qMax(qMax(iconImplicitHeight, textImplicitHeight), shortcutTextImplicitHeight);
    q->setImplicitSize(implicitWidth + horizontalPadding, implicitHeight + verticalPadding);
}

void QQuickMenuItemIconLabelPrivate::layout()
{
    Q_Q(QQuickIconLabel);
    if (!componentComplete)
        return;

    const qreal availableWidth = width - leftPadding - rightPadding;
    const qreal availableHeight = height - topPadding - bottomPadding;

    auto layoutShortcutLabel = [this, availableWidth, availableHeight](const qreal availableWidthForShortcut){
        if (availableWidthForShortcut - spacing - shortcutLabel->implicitWidth() > 0) {
            // There's enough space for everyone.
            shortcutLabel->setVisible(true);

            // We can simply align ourselves to the right of the entire available space.
            const QRectF shortcutTextRect = alignedRect(mirrored, Qt::AlignRight | Qt::AlignVCenter, QSizeF(
                    qMin(shortcutLabel->implicitWidth(), availableWidth),
                    qMin(shortcutLabel->implicitHeight(), availableHeight)),
                QRectF(leftPadding, topPadding, availableWidth, availableHeight));
            shortcutLabel->setSize(shortcutTextRect.size());
            shortcutLabel->setPosition(shortcutTextRect.topLeft());
        } else {
            // No space for us. As Widgets doesn't bother with eliding, neither do we; we just
            // don't show the shortcut if there's no room for it. The most important part of the
            // menu item is the text, anyway.
            shortcutLabel->setVisible(false);
        }
    };

    switch (display) {
    case QQuickIconLabel::IconOnly:
        QQuickIconLabelPrivate::layout();
        return;
    case QQuickIconLabel::TextOnly: {
        if (label) {
            const QRectF textRect = alignedRect(mirrored, alignment, QSizeF(
                    qMin(label->implicitWidth(), availableWidth),
                    qMin(label->implicitHeight(), availableHeight)),
                QRectF(leftPadding, topPadding, availableWidth, availableHeight));
            label->setSize(textRect.size());
            label->setPosition(textRect.topLeft());
        }

        if (shortcutLabel)
            layoutShortcutLabel(availableWidth - (label->width() + spacing));
        break;
    } case QQuickIconLabel::TextUnderIcon: {
        // TextUnderIcon doesn't make sense for menu items, but we don't need to break
        // any existing behaviour, so while we don't support showing shortcuts for it, we can
        // continue doing what we were doing before.
        QQuickIconLabelPrivate::layout();
        return;
    }
    case QQuickIconLabel::TextBesideIcon:
    default:
        QSizeF iconSize(0, 0);
        QSizeF textSize(0, 0);
        QSizeF shortcutLabelSize(0, 0);
        if (image) {
            iconSize.setWidth(qMin(image->implicitWidth(), availableWidth));
            iconSize.setHeight(qMin(image->implicitHeight(), availableHeight));
        }
        qreal spacingBetweenIconAndLabel = 0;
        if (label) {
            if (!iconSize.isEmpty())
                spacingBetweenIconAndLabel = spacing;
            textSize.setWidth(qMin(label->implicitWidth(), availableWidth - iconSize.width()
                - spacingBetweenIconAndLabel));
            textSize.setHeight(qMin(label->implicitHeight(), availableHeight));
        }

        qreal spacingBetweenLabelAndShortcutLabel = 0;
        if (shortcutLabel) {
            if (!textSize.isEmpty())
                spacingBetweenLabelAndShortcutLabel = spacing;
            shortcutLabelSize.setWidth(qMin(shortcutLabel->implicitWidth(), availableWidth - iconSize.width()
                - spacingBetweenIconAndLabel - textSize.width() - spacingBetweenLabelAndShortcutLabel));
            shortcutLabelSize.setHeight(qMin(shortcutLabel->implicitHeight(), availableHeight));
        }

        /*!
            For IconLabel, the layout would look like this:

            +-------+-+--------------+
            |       | |              |
            | Icon  | | Text         |
            |       | |              |
            +-------+-+--------------+

            For us, it looks like this:

            +-------+-+--------------+-+------------+
            |       | |              | |            |
            | Icon  | | Text         | | Shortcut   |
            |       | |              | |            |
            +-------+-+--------------+-+------------+
        */
        const QRectF combinedImageAndLabelRect = alignedRect(mirrored, alignment, QSizeF(
                iconSize.width() + spacingBetweenIconAndLabel + textSize.width(),
                qMax(iconSize.height(), textSize.height())),
            QRectF(leftPadding, topPadding, availableWidth, availableHeight));
        if (image) {
            const QRectF iconRect = alignedRect(mirrored, Qt::AlignLeft | Qt::AlignVCenter, iconSize, combinedImageAndLabelRect);
            image->setSize(iconRect.size());
            image->snapPositionTo(iconRect.topLeft());
        }
        if (label) {
            const QRectF textRect = alignedRect(mirrored, Qt::AlignRight | Qt::AlignVCenter, textSize, combinedImageAndLabelRect);
            label->setSize(textRect.size());
            label->setPosition(textRect.topLeft());
        }
        if (shortcutLabel)
            layoutShortcutLabel(availableWidth - combinedImageAndLabelRect.width());
        break;
    }

    q->setBaselineOffset(label ? label->y() + label->baselineOffset() : 0);
}

void QQuickMenuItemIconLabelPrivate::itemDestroyed(QQuickItem *item)
{
    QQuickIconLabelPrivate::itemDestroyed(item);

    if (item == shortcutLabel)
        shortcutLabel = nullptr;
}

void QQuickMenuItemIconLabelPrivate::textChange()
{
    updateOrSyncShortcutLabel();
}

void QQuickMenuItemIconLabelPrivate::displayChange()
{
    updateOrSyncShortcutLabel();
}

bool QQuickMenuItemIconLabelPrivate::updateShortcutLabel()
{
    if (!hasShortcut())
        return destroyShortcutLabel();
    return createShortcutLabel();
}

void QQuickMenuItemIconLabelPrivate::syncShortcutLabel()
{
    if (!shortcutLabel)
        return;

#if QT_CONFIG(shortcut)
    shortcutLabel->setText(shortcut().toString(QKeySequence::NativeText));
#endif
}

QQuickMenuItemIconLabel::~QQuickMenuItemIconLabel()
{
    Q_D(QQuickMenuItemIconLabel);
    if (d->shortcutLabel)
        d->unwatchChanges(d->shortcutLabel);
}

QQuickMenuItemIconLabel::QQuickMenuItemIconLabel(QQuickItem *parent)
    : QQuickIconLabel(*(new QQuickMenuItemIconLabelPrivate), parent)
{
}

QQuickMenuItem *QQuickMenuItemIconLabel::menuItem() const
{
    Q_D(const QQuickMenuItemIconLabel);
    return d->menuItem;
}

void QQuickMenuItemIconLabel::setMenuItem(QQuickMenuItem *menuItem)
{
    Q_D(QQuickMenuItemIconLabel);
    if (d->menuItem == menuItem)
        return;

    d->menuItem = menuItem;
    d->updateOrSyncShortcutLabel();
    emit menuItemChanged();
}

void QQuickMenuItemIconLabel::componentComplete()
{
    Q_D(QQuickMenuItemIconLabel);
    if (d->shortcutLabel)
        QQuickIconLabelPrivate::completeComponent(d->shortcutLabel);

    QQuickIconLabel::componentComplete();

    // See QQuickIconLabel::componentComplete for why we do this.
    const auto paintOrderChildItems = QQuickItemPrivate::get(this)->paintOrderChildItems();
    const auto bottomMostFosterChildIt = std::find_if(paintOrderChildItems.constBegin(),
        paintOrderChildItems.constEnd(), [d](QQuickItem *item) {
            return item != d->label && item != d->image && item != d->shortcutLabel;
        });
    if (bottomMostFosterChildIt != paintOrderChildItems.constEnd()) {
        const QQuickItem *bottomMostFosterChild = *bottomMostFosterChildIt;
        if (d->shortcutLabel)
            d->shortcutLabel->stackBefore(bottomMostFosterChild);
    }
}

QT_END_NAMESPACE

#include "moc_qquickmenuitemiconlabel_p.cpp"
