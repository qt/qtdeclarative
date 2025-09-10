// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstyleitemprogressbar.h"

QT_BEGIN_NAMESPACE

QFont QQuickStyleItemProgressBar::styleFont(QQuickItem *control) const
{
    return style()->font(QStyle::CE_ProgressBarLabel, controlSize(control));
}

void QQuickStyleItemProgressBar::connectToControl() const
{
    QQuickStyleItem::connectToControl();
    auto progressBar = control<QQuickProgressBar>();
    connect(progressBar, &QQuickProgressBar::fromChanged, this, &QQuickStyleItem::markImageDirty);
    connect(progressBar, &QQuickProgressBar::toChanged, this, &QQuickStyleItem::markImageDirty);
    connect(progressBar, &QQuickProgressBar::positionChanged, this, &QQuickStyleItem::markImageDirty);
}

StyleItemGeometry QQuickStyleItemProgressBar::calculateGeometry()
{
    QStyleOptionProgressBar styleOption;
    initStyleOption(styleOption);

    StyleItemGeometry geometry;
    geometry.minimumSize = style()->sizeFromContents(QStyle::CT_ProgressBar, &styleOption, QSize(0, 0));

    // From qprogressbar.cpp in qtbase:
    const int cw = style()->pixelMetric(QStyle::PM_ProgressBarChunkWidth, &styleOption);
    QFontMetrics fm(control<QQuickProgressBar>()->font());
    QSize size = QSize(qMax(9, cw) * 7 + fm.horizontalAdvance(QLatin1Char('0')) * 4, fm.height() + 8);
    if (!(styleOption.state & QStyle::State_Horizontal))
        size = size.transposed();

    geometry.implicitSize =  style()->sizeFromContents(QStyle::CT_ProgressBar, &styleOption, size);
    styleOption.rect = QRect(QPoint(0, 0), geometry.implicitSize);
    geometry.contentRect = style()->subElementRect(QStyle::SE_ProgressBarContents, &styleOption);
    geometry.layoutRect = style()->subElementRect(QStyle::SE_ProgressBarLayoutItem, &styleOption);
    geometry.ninePatchMargins = style()->ninePatchMargins(QStyle::CE_ProgressBar, &styleOption, geometry.minimumSize);

    return geometry;
}

void QQuickStyleItemProgressBar::paintEvent(QPainter *painter) const
{
    QStyleOptionProgressBar styleOption;
    initStyleOption(styleOption);
#ifndef Q_OS_MACOS
    const QRect r = styleOption.rect;
#endif
    // Note: on macOS, the groove will paint both the background and the contents
    styleOption.rect = style()->subElementRect(QStyle::SE_ProgressBarGroove, &styleOption);
    style()->drawControl(QStyle::CE_ProgressBarGroove, &styleOption, painter);
#ifndef Q_OS_MACOS
    styleOption.rect = r;
    styleOption.rect = style()->subElementRect(QStyle::SE_ProgressBarContents, &styleOption);
    style()->drawControl(QStyle::CE_ProgressBarContents, &styleOption, painter);
#endif
}

void QQuickStyleItemProgressBar::initStyleOption(QStyleOptionProgressBar &styleOption) const
{
    initStyleOptionBase(styleOption);
    auto progressBar = control<QQuickProgressBar>();

    styleOption.state = QStyle::State_Horizontal;

    if (progressBar->isIndeterminate()) {
        styleOption.minimum = 0;
        styleOption.maximum = 0;
    } else if (progressBar->to() - progressBar->from() < 100) {
        // Add some range to support float numbers
        styleOption.minimum = 0;
        styleOption.maximum = (progressBar->to() - progressBar->from()) * 100;
        styleOption.progress = (progressBar->value() - progressBar->from()) * 100;
    } else {
        styleOption.minimum = progressBar->from();
        styleOption.maximum = progressBar->to();
        styleOption.progress = progressBar->value();
    }
}

QT_END_NAMESPACE

#include "moc_qquickstyleitemprogressbar.cpp"
