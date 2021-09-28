/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Dialogs module of the Qt Toolkit.
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

#ifndef QQUICKFONTDIALOGIMPL_P_H
#define QQUICKFONTDIALOGIMPL_P_H

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

#include <QtGui/qfontdatabase.h>
#include <QtQuick/private/qquicklistview_p.h>
#include <QtQuick/private/qquicktextedit_p.h>
#include <QtQuickTemplates2/private/qquicktextfield_p.h>
#include <QtQuickTemplates2/private/qquickcombobox_p.h>
#include <QtQuickTemplates2/private/qquickcheckbox_p.h>
#include <QtQuickTemplates2/private/qquickdialog_p.h>
#include "qtquickdialogs2quickimplglobal_p.h"

QT_BEGIN_NAMESPACE

class QQuickDialogButtonBox;

class QQuickFontDialogImplAttached;
class QQuickFontDialogImplAttachedPrivate;
class QQuickFontDialogImplPrivate;

class Q_QUICKDIALOGS2QUICKIMPL_PRIVATE_EXPORT QQuickFontDialogImpl : public QQuickDialog
{
    Q_OBJECT
    Q_PROPERTY(QFont currentFont READ currentFont WRITE setCurrentFont NOTIFY currentFontChanged FINAL)
    QML_NAMED_ELEMENT(FontDialogImpl)
    QML_ATTACHED(QQuickFontDialogImplAttached)
    QML_ADDED_IN_VERSION(6, 2)

public:
    explicit QQuickFontDialogImpl(QObject *parent = nullptr);

    static QQuickFontDialogImplAttached *qmlAttachedProperties(QObject *object);

    QSharedPointer<QFontDialogOptions> options() const;
    void setOptions(const QSharedPointer<QFontDialogOptions> &options);

    QFont currentFont() const;
    void setCurrentFont(const QFont &font, bool selectInListViews = false);

    void init();

Q_SIGNALS:
    void optionsChanged();
    void currentFontChanged(const QFont &font);
    void fontSelected(const QFont &font);

private:
    void keyReleaseEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

    Q_DISABLE_COPY(QQuickFontDialogImpl)
    Q_DECLARE_PRIVATE(QQuickFontDialogImpl)
};

class Q_QUICKDIALOGS2QUICKIMPL_PRIVATE_EXPORT QQuickFontDialogImplAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickListView *familyListView READ familyListView WRITE setFamilyListView
               NOTIFY familyListViewChanged)
    Q_PROPERTY(QQuickListView *styleListView READ styleListView WRITE setStyleListView
               NOTIFY styleListViewChanged)
    Q_PROPERTY(QQuickListView *sizeListView READ sizeListView WRITE setSizeListView
               NOTIFY sizeListViewChanged)
    Q_PROPERTY(QQuickTextEdit *sampleEdit READ sampleEdit WRITE setSampleEdit
               NOTIFY sampleEditChanged)
    Q_PROPERTY(QQuickDialogButtonBox *buttonBox READ buttonBox WRITE setButtonBox
               NOTIFY buttonBoxChanged)
    Q_PROPERTY(QQuickComboBox *writingSystemComboBox READ writingSystemComboBox
               WRITE setWritingSystemComboBox NOTIFY writingSystemComboBoxChanged)
    Q_PROPERTY(QQuickCheckBox *underlineCheckBox READ underlineCheckBox WRITE setUnderlineCheckBox
                       NOTIFY underlineCheckBoxChanged)
    Q_PROPERTY(QQuickCheckBox *strikeoutCheckBox READ strikeoutCheckBox WRITE setStrikeoutCheckBox
                       NOTIFY strikeoutCheckBoxChanged)

    Q_PROPERTY(QQuickTextField *familyEdit READ familyEdit WRITE setFamilyEdit
               NOTIFY familyEditChanged)
    Q_PROPERTY(QQuickTextField *styleEdit READ styleEdit WRITE setStyleEdit NOTIFY styleEditChanged)
    Q_PROPERTY(QQuickTextField *sizeEdit READ sizeEdit WRITE setSizeEdit NOTIFY sizeEditChanged)

    Q_MOC_INCLUDE(<QtQuickTemplates2 / private / qquickdialogbuttonbox_p.h>)

public:
    explicit QQuickFontDialogImplAttached(QObject *parent = nullptr);

    QQuickListView *familyListView() const;
    void setFamilyListView(QQuickListView *familyListView);

    QQuickListView *styleListView() const;
    void setStyleListView(QQuickListView *styleListView);

    QQuickListView *sizeListView() const;
    void setSizeListView(QQuickListView *sizeListView);

    QQuickTextEdit *sampleEdit() const;
    void setSampleEdit(QQuickTextEdit *sampleEdit);

    QQuickDialogButtonBox *buttonBox() const;
    void setButtonBox(QQuickDialogButtonBox *buttonBox);

    QQuickComboBox *writingSystemComboBox() const;
    void setWritingSystemComboBox(QQuickComboBox *writingSystemComboBox);

    QQuickCheckBox *underlineCheckBox() const;
    void setUnderlineCheckBox(QQuickCheckBox *underlineCheckBox);

    QQuickCheckBox *strikeoutCheckBox() const;
    void setStrikeoutCheckBox(QQuickCheckBox *strikethroughCheckBox);

    QQuickTextField *familyEdit() const;
    void setFamilyEdit(QQuickTextField *familyEdit);

    QQuickTextField *styleEdit() const;
    void setStyleEdit(QQuickTextField *styleEdit);

    QQuickTextField *sizeEdit() const;
    void setSizeEdit(QQuickTextField *sizeEdit);

Q_SIGNALS:
    void buttonBoxChanged();
    void familyListViewChanged();
    void styleListViewChanged();
    void sizeListViewChanged();
    void sampleEditChanged();
    void writingSystemComboBoxChanged();
    void underlineCheckBoxChanged();
    void strikeoutCheckBoxChanged();
    void familyEditChanged();
    void styleEditChanged();
    void sizeEditChanged();

public:
    void searchFamily(const QString &s) { searchListView(s, familyListView()); }
    void searchStyle(const QString &s) { searchListView(s, styleListView()); }
    void clearSearch();

    void updateFamilies();
    void selectFontInListViews(const QFont &font);

private:
    void updateStyles();
    void updateSizes();

    void _q_familyChanged();
    void _q_styleChanged();
    void _q_sizeEdited();
    void _q_sizeChanged();
    void _q_updateSample();

    void _q_writingSystemChanged(int index);

    void searchListView(const QString &s, QQuickListView *listView);

    QFontDatabase::WritingSystem m_writingSystem;
    QString m_selectedFamily;
    QString m_selectedStyle;
    QString m_search;
    int m_selectedSize;
    bool m_smoothlyScalable;
    bool m_ignoreFamilyUpdate;
    bool m_ignoreStyleUpdate;

    Q_DISABLE_COPY(QQuickFontDialogImplAttached)
    Q_DECLARE_PRIVATE(QQuickFontDialogImplAttached)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickFontDialogImpl)

#endif // QQUICKFONTDIALOGIMPL_P_H
