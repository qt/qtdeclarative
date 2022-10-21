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
#ifndef QQUICKPALETTE_H
#define QQUICKPALETTE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of QQuickPalette. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <vector> // Workaround: I think we should include vector to qflatmap_p.h
#include <QtCore/private/qflatmap_p.h>

#include <QtQuick/private/qquickcolorgroup_p.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractPaletteProvider;

class Q_QUICK_PRIVATE_EXPORT QQuickPalette : public QQuickColorGroup
{
    Q_OBJECT

    Q_PROPERTY(QQuickColorGroup *active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QQuickColorGroup *inactive READ inactive WRITE setInactive NOTIFY inactiveChanged)
    Q_PROPERTY(QQuickColorGroup *disabled READ disabled WRITE setDisabled NOTIFY disabledChanged)

    QML_NAMED_ELEMENT(Palette)
    QML_ADDED_IN_VERSION(6, 0)

public: // Types
    using PalettePtr = QPointer<QQuickPalette>;

public:
    Q_DISABLE_COPY_MOVE(QQuickPalette)
    explicit QQuickPalette(QObject *parent = nullptr);

    QQuickColorGroup *active() const;
    QQuickColorGroup *inactive() const;
    QQuickColorGroup *disabled() const;

    QPalette::ColorGroup currentColorGroup() const override;
    void setCurrentGroup(QPalette::ColorGroup currentGroup);

    void fromQPalette(QPalette palette);
    QPalette toQPalette() const;

    const QQuickAbstractPaletteProvider *paletteProvider() const;
    void setPaletteProvider(const QQuickAbstractPaletteProvider *paletteProvider);

    void reset();

    void inheritPalette(const QPalette &palette);

public Q_SLOTS:
    void setActive(QQuickColorGroup *active);
    void setInactive(QQuickColorGroup *inactive);
    void setDisabled(QQuickColorGroup *disabled);

Q_SIGNALS:
    void activeChanged();
    void inactiveChanged();
    void disabledChanged();

private:
    void setColorGroup(QPalette::ColorGroup groupTag,
                       const QQuickColorGroup::GroupPtr &group,
                       void (QQuickPalette::*notifier)());
    QQuickColorGroup::GroupPtr colorGroup(QPalette::ColorGroup groupTag) const;
    QQuickColorGroup::GroupPtr findColorGroup(QPalette::ColorGroup groupTag) const;

    void registerColorGroup(QQuickColorGroup *group, QPalette::ColorGroup groupTag);

    bool isValidColorGroup(QPalette::ColorGroup groupTag,
                           const QQuickColorGroup::GroupPtr &colorGroup) const;

    static constexpr QPalette::ColorGroup defaultCurrentGroup() { return QPalette::Active; }

private:
    QFlatMap<QPalette::ColorGroup, QQuickColorGroup::GroupPtr> m_colorGroups;
    QPalette::ColorGroup m_currentGroup;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickPalette)

#endif // QQUICKPALETTE_H
