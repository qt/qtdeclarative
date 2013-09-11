/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick.Dialogs module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKABSTRACTFONTDIALOG_P_H
#define QQUICKABSTRACTFONTDIALOG_P_H

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

#include <QtQml>
#include <QQuickView>
#include <QtGui/qpa/qplatformdialoghelper.h>
#include <QtGui/qfont.h>
#include <qpa/qplatformtheme.h>
#include "qquickabstractdialog_p.h"

QT_BEGIN_NAMESPACE

class QQuickAbstractFontDialog : public QQuickAbstractDialog
{
    Q_OBJECT
    Q_PROPERTY(bool scalableFonts READ scalableFonts WRITE setScalableFonts NOTIFY scalableFontsChanged)
    Q_PROPERTY(bool nonScalableFonts READ nonScalableFonts WRITE setNonScalableFonts NOTIFY nonScalableFontsChanged)
    Q_PROPERTY(bool monospacedFonts READ monospacedFonts WRITE setMonospacedFonts NOTIFY monospacedFontsChanged)
    Q_PROPERTY(bool proportionalFonts READ proportionalFonts WRITE setProportionalFonts NOTIFY proportionalFontsChanged)
    Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)

public:
    QQuickAbstractFontDialog(QObject *parent = 0);
    virtual ~QQuickAbstractFontDialog();

    virtual QString title() const;
    bool scalableFonts() const;
    bool nonScalableFonts() const;
    bool monospacedFonts() const;
    bool proportionalFonts() const;
    QFont font() const { return m_font; }

public Q_SLOTS:
    void setVisible(bool v);
    void setModality(Qt::WindowModality m);
    void setTitle(const QString &t);
    void setFont(const QFont &arg);
    void setScalableFonts(bool arg);
    void setNonScalableFonts(bool arg);
    void setMonospacedFonts(bool arg);
    void setProportionalFonts(bool arg);

Q_SIGNALS:
    void scalableFontsChanged();
    void nonScalableFontsChanged();
    void monospacedFontsChanged();
    void proportionalFontsChanged();
    void fontChanged();
    void selectionAccepted();

protected:
    QPlatformFontDialogHelper *m_dlgHelper;
    QSharedPointer<QFontDialogOptions> m_options;
    QFont m_font;

    Q_DISABLE_COPY(QQuickAbstractFontDialog)
};

QT_END_NAMESPACE

#endif // QQUICKABSTRACTFONTDIALOG_P_H
