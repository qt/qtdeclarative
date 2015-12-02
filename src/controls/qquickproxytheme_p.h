/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Controls module of the Qt Toolkit.
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

#ifndef QQUICKPROXYTHEME_H
#define QQUICKPROXYTHEME_H

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

#include <QtGui/qpa/qplatformtheme.h>

QT_BEGIN_NAMESPACE

class QQuickProxyTheme :  public QPlatformTheme
{
public:
    QQuickProxyTheme(QPlatformTheme *theme);

    ~QQuickProxyTheme();

    QPlatformTheme* theme() const;

    QPlatformMenuItem* createPlatformMenuItem() const Q_DECL_OVERRIDE;
    QPlatformMenu* createPlatformMenu() const Q_DECL_OVERRIDE;
    QPlatformMenuBar* createPlatformMenuBar() const Q_DECL_OVERRIDE;
    void showPlatformMenuBar() Q_DECL_OVERRIDE;

    bool usePlatformNativeDialog(DialogType type) const Q_DECL_OVERRIDE;
    QPlatformDialogHelper *createPlatformDialogHelper(DialogType type) const Q_DECL_OVERRIDE;

#ifndef QT_NO_SYSTEMTRAYICON
    QPlatformSystemTrayIcon *createPlatformSystemTrayIcon() const Q_DECL_OVERRIDE;
#endif

    const QPalette *palette(Palette type = SystemPalette) const Q_DECL_OVERRIDE;

    const QFont *font(Font type = SystemFont) const Q_DECL_OVERRIDE;

    QVariant themeHint(ThemeHint hint) const Q_DECL_OVERRIDE;

    QPixmap standardPixmap(StandardPixmap sp, const QSizeF &size) const Q_DECL_OVERRIDE;
    QPixmap fileIconPixmap(const QFileInfo &fileInfo, const QSizeF &size,
                                   QPlatformTheme::IconOptions iconOptions = 0) const Q_DECL_OVERRIDE;

    QIconEngine *createIconEngine(const QString &iconName) const Q_DECL_OVERRIDE;

    QList<QKeySequence> keyBindings(QKeySequence::StandardKey key) const Q_DECL_OVERRIDE;

    QString standardButtonText(int button) const Q_DECL_OVERRIDE;

private:
    QPlatformTheme *m_theme;
};

QT_END_NAMESPACE

#endif // QQUICKPROXYTHEME_H
