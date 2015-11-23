/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
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

#include "qquickmaterialstyle_p.h"
#include "qquickstyle_p.h"

#include <QtCore/qset.h>
#include <QtCore/qpointer.h>
#include <QtQml/qqmlengine.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Material
    \inherits QtObject
    \instantiates QQuickMaterialStyleAttached
    \inqmlmodule QtQuick.Controls.Material
    \ingroup utilities
    \brief A style interface.

    TODO
*/

/*!
    \qmlattachedproperty color QtQuickControls2::Material::textColorPrimaray
*/

struct MaterialColor
{
    MaterialColor() :
        color(QQuickMaterialStyle::Red),
        shade(QQuickMaterialStyle::Shade500)
    {
    }

    MaterialColor(QQuickMaterialStyle::Color color, QQuickMaterialStyle::Shade shade) :
        color(color),
        shade(shade)
    {
    }

    QQuickMaterialStyle::Color color;
    QQuickMaterialStyle::Shade shade;
};

inline bool operator==(const MaterialColor &lhs, const MaterialColor &rhs)
{
    return lhs.color == rhs.color && lhs.shade == rhs.shade;
}

inline uint qHash(const MaterialColor &color, uint seed)
{
    return qHash(color.color, seed) ^ color.shade;
}

class QQuickMaterialStylePrivate : public QObjectPrivate, public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickMaterialStyle)

public:
    QQuickMaterialStylePrivate();

    enum Method { Implicit, Explicit, Inherit };

    QPointer<QQuickMaterialStyle> parentStyle;
    QSet<QQuickMaterialStyle *> childStyles;
    QHash<MaterialColor, QColor> colors;

    bool explicitTheme;
    bool explicitPrimary;
    bool explicitAccent;
    QQuickMaterialStyle::Theme theme;
    QQuickMaterialStyle::Color primary;
    QQuickMaterialStyle::Color accent;
};

static const QQuickMaterialStyle::Theme defaultTheme = QQuickMaterialStyle::Light;
static const QQuickMaterialStyle::Color defaultPrimary = QQuickMaterialStyle::BlueGrey;
static const QQuickMaterialStyle::Color defaultAccent = QQuickMaterialStyle::Teal;
static const QColor backgroundColorLight = "#FFFAFAFA";
static const QColor backgroundColorDark = "#FF303030";
static const QColor dialogColorLight = "#FFFFFFFF";
static const QColor dialogColorDark = "#FF303030";
static const QColor primaryTextColorLight = "#DD000000";
static const QColor primaryTextColorDark = "#FFFFFFFF";
static const QColor secondaryTextColorLight = "#89000000";
static const QColor secondaryTextColorDark = "#B2FFFFFF";
static const QColor hintTextColorLight = "#60000000";
static const QColor hintTextColorDark = "#4CFFFFFF";
static const QColor dividerTextColorLight = "#1E000000";
static const QColor dividerTextColorDark = "#1EFFFFFF";
static const QColor raisedButtonColorLight = "#FFD6D7D7";
// TODO: find out actual value
static const QColor raisedButtonPressColorLight = "#FFCCCDCD";
static const QColor raisedButtonDisabledColorLight = dividerTextColorLight;
static const QColor raisedButtonDisabledColorDark = dividerTextColorDark;
static const QColor flatButtonPressColorLight = "#66999999";
static const QColor flatButtonPressColorDark = "#3FCCCCCC";
static const QColor flatButtonFocusColorLight = "#33CCCCCC";
static const QColor flatButtonFocusColorDark = "#26CCCCCC";
static const QColor frameColorLight = hintTextColorLight;
static const QColor frameColorDark = hintTextColorDark;
static const QColor switchUncheckedTrackColorLight = "#42000000";
static const QColor switchUncheckedTrackColorDark = "#4CFFFFFF";
static const QColor switchDisabledTrackColorLight = "#1E000000";
static const QColor switchDisabledTrackColorDark = "#19FFFFFF";
// TODO: find out actual values
static const QColor checkBoxUncheckedRippleColorLight = "#10000000";
static const QColor checkBoxUncheckedRippleColorDark = "#20FFFFFF";

QQuickMaterialStylePrivate::QQuickMaterialStylePrivate() :
    explicitTheme(false),
    explicitPrimary(false),
    explicitAccent(false),
    theme(defaultTheme),
    primary(defaultPrimary),
    accent(defaultAccent)
{
    if (colors.isEmpty()) {
        typedef QQuickMaterialStyle Style;
        colors[MaterialColor(Style::Red, Style::Shade50)] = "#FFEBEE";
        colors[MaterialColor(Style::Red, Style::Shade100)] = "#FFCDD2";
        colors[MaterialColor(Style::Red, Style::Shade200)] = "#EF9A9A";
        colors[MaterialColor(Style::Red, Style::Shade300)] = "#E57373";
        colors[MaterialColor(Style::Red, Style::Shade400)] = "#EF5350";
        colors[MaterialColor(Style::Red, Style::Shade500)] = "#F44336";
        colors[MaterialColor(Style::Red, Style::Shade600)] = "#E53935";
        colors[MaterialColor(Style::Red, Style::Shade700)] = "#D32F2F";
        colors[MaterialColor(Style::Red, Style::Shade800)] = "#C62828";
        colors[MaterialColor(Style::Red, Style::Shade900)] = "#B71C1C";
        colors[MaterialColor(Style::Red, Style::ShadeA100)] = "#FF8A80";
        colors[MaterialColor(Style::Red, Style::ShadeA200)] = "#FF5252";
        colors[MaterialColor(Style::Red, Style::ShadeA400)] = "#FF1744";
        colors[MaterialColor(Style::Red, Style::ShadeA700)] = "#D50000";
        colors[MaterialColor(Style::Pink, Style::Shade50)] = "#FCE4EC";
        colors[MaterialColor(Style::Pink, Style::Shade100)] = "#F8BBD0";
        colors[MaterialColor(Style::Pink, Style::Shade200)] = "#F48FB1";
        colors[MaterialColor(Style::Pink, Style::Shade300)] = "#F06292";
        colors[MaterialColor(Style::Pink, Style::Shade400)] = "#EC407A";
        colors[MaterialColor(Style::Pink, Style::Shade500)] = "#E91E63";
        colors[MaterialColor(Style::Pink, Style::Shade600)] = "#D81B60";
        colors[MaterialColor(Style::Pink, Style::Shade700)] = "#C2185B";
        colors[MaterialColor(Style::Pink, Style::Shade800)] = "#AD1457";
        colors[MaterialColor(Style::Pink, Style::Shade900)] = "#880E4F";
        colors[MaterialColor(Style::Pink, Style::ShadeA100)] = "#FF80AB";
        colors[MaterialColor(Style::Pink, Style::ShadeA200)] = "#FF4081";
        colors[MaterialColor(Style::Pink, Style::ShadeA400)] = "#F50057";
        colors[MaterialColor(Style::Pink, Style::ShadeA700)] = "#C51162";
        colors[MaterialColor(Style::Purple, Style::Shade50)] = "#F3E5F5";
        colors[MaterialColor(Style::Purple, Style::Shade100)] = "#E1BEE7";
        colors[MaterialColor(Style::Purple, Style::Shade200)] = "#CE93D8";
        colors[MaterialColor(Style::Purple, Style::Shade300)] = "#BA68C8";
        colors[MaterialColor(Style::Purple, Style::Shade400)] = "#AB47BC";
        colors[MaterialColor(Style::Purple, Style::Shade500)] = "#9C27B0";
        colors[MaterialColor(Style::Purple, Style::Shade600)] = "#8E24AA";
        colors[MaterialColor(Style::Purple, Style::Shade700)] = "#7B1FA2";
        colors[MaterialColor(Style::Purple, Style::Shade800)] = "#6A1B9A";
        colors[MaterialColor(Style::Purple, Style::Shade900)] = "#4A148C";
        colors[MaterialColor(Style::Purple, Style::ShadeA100)] = "#EA80FC";
        colors[MaterialColor(Style::Purple, Style::ShadeA200)] = "#E040FB";
        colors[MaterialColor(Style::Purple, Style::ShadeA400)] = "#D500F9";
        colors[MaterialColor(Style::Purple, Style::ShadeA700)] = "#AA00FF";
        colors[MaterialColor(Style::DeepPurple, Style::Shade50)] = "#EDE7F6";
        colors[MaterialColor(Style::DeepPurple, Style::Shade100)] = "#D1C4E9";
        colors[MaterialColor(Style::DeepPurple, Style::Shade200)] = "#B39DDB";
        colors[MaterialColor(Style::DeepPurple, Style::Shade300)] = "#9575CD";
        colors[MaterialColor(Style::DeepPurple, Style::Shade400)] = "#7E57C2";
        colors[MaterialColor(Style::DeepPurple, Style::Shade500)] = "#673AB7";
        colors[MaterialColor(Style::DeepPurple, Style::Shade600)] = "#5E35B1";
        colors[MaterialColor(Style::DeepPurple, Style::Shade700)] = "#512DA8";
        colors[MaterialColor(Style::DeepPurple, Style::Shade800)] = "#4527A0";
        colors[MaterialColor(Style::DeepPurple, Style::Shade900)] = "#311B92";
        colors[MaterialColor(Style::DeepPurple, Style::ShadeA100)] = "#B388FF";
        colors[MaterialColor(Style::DeepPurple, Style::ShadeA200)] = "#7C4DFF";
        colors[MaterialColor(Style::DeepPurple, Style::ShadeA400)] = "#651FFF";
        colors[MaterialColor(Style::DeepPurple, Style::ShadeA700)] = "#6200EA";
        colors[MaterialColor(Style::Indigo, Style::Shade50)] = "#E8EAF6";
        colors[MaterialColor(Style::Indigo, Style::Shade100)] = "#C5CAE9";
        colors[MaterialColor(Style::Indigo, Style::Shade200)] = "#9FA8DA";
        colors[MaterialColor(Style::Indigo, Style::Shade300)] = "#7986CB";
        colors[MaterialColor(Style::Indigo, Style::Shade400)] = "#5C6BC0";
        colors[MaterialColor(Style::Indigo, Style::Shade500)] = "#3F51B5";
        colors[MaterialColor(Style::Indigo, Style::Shade600)] = "#3949AB";
        colors[MaterialColor(Style::Indigo, Style::Shade700)] = "#303F9F";
        colors[MaterialColor(Style::Indigo, Style::Shade800)] = "#283593";
        colors[MaterialColor(Style::Indigo, Style::Shade900)] = "#1A237E";
        colors[MaterialColor(Style::Indigo, Style::ShadeA100)] = "#8C9EFF";
        colors[MaterialColor(Style::Indigo, Style::ShadeA200)] = "#536DFE";
        colors[MaterialColor(Style::Indigo, Style::ShadeA400)] = "#3D5AFE";
        colors[MaterialColor(Style::Indigo, Style::ShadeA700)] = "#304FFE";
        colors[MaterialColor(Style::Blue, Style::Shade50)] = "#E3F2FD";
        colors[MaterialColor(Style::Blue, Style::Shade100)] = "#BBDEFB";
        colors[MaterialColor(Style::Blue, Style::Shade200)] = "#90CAF9";
        colors[MaterialColor(Style::Blue, Style::Shade300)] = "#64B5F6";
        colors[MaterialColor(Style::Blue, Style::Shade400)] = "#42A5F5";
        colors[MaterialColor(Style::Blue, Style::Shade500)] = "#2196F3";
        colors[MaterialColor(Style::Blue, Style::Shade600)] = "#1E88E5";
        colors[MaterialColor(Style::Blue, Style::Shade700)] = "#1976D2";
        colors[MaterialColor(Style::Blue, Style::Shade800)] = "#1565C0";
        colors[MaterialColor(Style::Blue, Style::Shade900)] = "#0D47A1";
        colors[MaterialColor(Style::Blue, Style::ShadeA100)] = "#82B1FF";
        colors[MaterialColor(Style::Blue, Style::ShadeA200)] = "#448AFF";
        colors[MaterialColor(Style::Blue, Style::ShadeA400)] = "#2979FF";
        colors[MaterialColor(Style::Blue, Style::ShadeA700)] = "#2962FF";
        colors[MaterialColor(Style::LightBlue, Style::Shade50)] = "#E1F5FE";
        colors[MaterialColor(Style::LightBlue, Style::Shade100)] = "#B3E5FC";
        colors[MaterialColor(Style::LightBlue, Style::Shade200)] = "#81D4FA";
        colors[MaterialColor(Style::LightBlue, Style::Shade300)] = "#4FC3F7";
        colors[MaterialColor(Style::LightBlue, Style::Shade400)] = "#29B6F6";
        colors[MaterialColor(Style::LightBlue, Style::Shade500)] = "#03A9F4";
        colors[MaterialColor(Style::LightBlue, Style::Shade600)] = "#039BE5";
        colors[MaterialColor(Style::LightBlue, Style::Shade700)] = "#0288D1";
        colors[MaterialColor(Style::LightBlue, Style::Shade800)] = "#0277BD";
        colors[MaterialColor(Style::LightBlue, Style::Shade900)] = "#01579B";
        colors[MaterialColor(Style::LightBlue, Style::ShadeA100)] = "#80D8FF";
        colors[MaterialColor(Style::LightBlue, Style::ShadeA200)] = "#40C4FF";
        colors[MaterialColor(Style::LightBlue, Style::ShadeA400)] = "#00B0FF";
        colors[MaterialColor(Style::LightBlue, Style::ShadeA700)] = "#0091EA";
        colors[MaterialColor(Style::Cyan, Style::Shade50)] = "#E0F7FA";
        colors[MaterialColor(Style::Cyan, Style::Shade100)] = "#B2EBF2";
        colors[MaterialColor(Style::Cyan, Style::Shade200)] = "#80DEEA";
        colors[MaterialColor(Style::Cyan, Style::Shade300)] = "#4DD0E1";
        colors[MaterialColor(Style::Cyan, Style::Shade400)] = "#26C6DA";
        colors[MaterialColor(Style::Cyan, Style::Shade500)] = "#00BCD4";
        colors[MaterialColor(Style::Cyan, Style::Shade600)] = "#00ACC1";
        colors[MaterialColor(Style::Cyan, Style::Shade700)] = "#0097A7";
        colors[MaterialColor(Style::Cyan, Style::Shade800)] = "#00838F";
        colors[MaterialColor(Style::Cyan, Style::Shade900)] = "#006064";
        colors[MaterialColor(Style::Cyan, Style::ShadeA100)] = "#84FFFF";
        colors[MaterialColor(Style::Cyan, Style::ShadeA200)] = "#18FFFF";
        colors[MaterialColor(Style::Cyan, Style::ShadeA400)] = "#00E5FF";
        colors[MaterialColor(Style::Cyan, Style::ShadeA700)] = "#00B8D4";
        colors[MaterialColor(Style::Teal, Style::Shade50)] = "#E0F2F1";
        colors[MaterialColor(Style::Teal, Style::Shade100)] = "#B2DFDB";
        colors[MaterialColor(Style::Teal, Style::Shade200)] = "#80CBC4";
        colors[MaterialColor(Style::Teal, Style::Shade300)] = "#4DB6AC";
        colors[MaterialColor(Style::Teal, Style::Shade400)] = "#26A69A";
        colors[MaterialColor(Style::Teal, Style::Shade500)] = "#009688";
        colors[MaterialColor(Style::Teal, Style::Shade600)] = "#00897B";
        colors[MaterialColor(Style::Teal, Style::Shade700)] = "#00796B";
        colors[MaterialColor(Style::Teal, Style::Shade800)] = "#00695C";
        colors[MaterialColor(Style::Teal, Style::Shade900)] = "#004D40";
        colors[MaterialColor(Style::Teal, Style::ShadeA100)] = "#A7FFEB";
        colors[MaterialColor(Style::Teal, Style::ShadeA200)] = "#64FFDA";
        colors[MaterialColor(Style::Teal, Style::ShadeA400)] = "#1DE9B6";
        colors[MaterialColor(Style::Teal, Style::ShadeA700)] = "#00BFA5";
        colors[MaterialColor(Style::Green, Style::Shade50)] = "#E8F5E9";
        colors[MaterialColor(Style::Green, Style::Shade100)] = "#C8E6C9";
        colors[MaterialColor(Style::Green, Style::Shade200)] = "#A5D6A7";
        colors[MaterialColor(Style::Green, Style::Shade300)] = "#81C784";
        colors[MaterialColor(Style::Green, Style::Shade400)] = "#66BB6A";
        colors[MaterialColor(Style::Green, Style::Shade500)] = "#4CAF50";
        colors[MaterialColor(Style::Green, Style::Shade600)] = "#43A047";
        colors[MaterialColor(Style::Green, Style::Shade700)] = "#388E3C";
        colors[MaterialColor(Style::Green, Style::Shade800)] = "#2E7D32";
        colors[MaterialColor(Style::Green, Style::Shade900)] = "#1B5E20";
        colors[MaterialColor(Style::Green, Style::ShadeA100)] = "#B9F6CA";
        colors[MaterialColor(Style::Green, Style::ShadeA200)] = "#69F0AE";
        colors[MaterialColor(Style::Green, Style::ShadeA400)] = "#00E676";
        colors[MaterialColor(Style::Green, Style::ShadeA700)] = "#00C853";
        colors[MaterialColor(Style::LightGreen, Style::Shade50)] = "#F1F8E9";
        colors[MaterialColor(Style::LightGreen, Style::Shade100)] = "#DCEDC8";
        colors[MaterialColor(Style::LightGreen, Style::Shade200)] = "#C5E1A5";
        colors[MaterialColor(Style::LightGreen, Style::Shade300)] = "#AED581";
        colors[MaterialColor(Style::LightGreen, Style::Shade400)] = "#9CCC65";
        colors[MaterialColor(Style::LightGreen, Style::Shade500)] = "#8BC34A";
        colors[MaterialColor(Style::LightGreen, Style::Shade600)] = "#7CB342";
        colors[MaterialColor(Style::LightGreen, Style::Shade700)] = "#689F38";
        colors[MaterialColor(Style::LightGreen, Style::Shade800)] = "#558B2F";
        colors[MaterialColor(Style::LightGreen, Style::Shade900)] = "#33691E";
        colors[MaterialColor(Style::LightGreen, Style::ShadeA100)] = "#CCFF90";
        colors[MaterialColor(Style::LightGreen, Style::ShadeA200)] = "#B2FF59";
        colors[MaterialColor(Style::LightGreen, Style::ShadeA400)] = "#76FF03";
        colors[MaterialColor(Style::LightGreen, Style::ShadeA700)] = "#64DD17";
        colors[MaterialColor(Style::Lime, Style::Shade50)] = "#F9FBE7";
        colors[MaterialColor(Style::Lime, Style::Shade100)] = "#F0F4C3";
        colors[MaterialColor(Style::Lime, Style::Shade200)] = "#E6EE9C";
        colors[MaterialColor(Style::Lime, Style::Shade300)] = "#DCE775";
        colors[MaterialColor(Style::Lime, Style::Shade400)] = "#D4E157";
        colors[MaterialColor(Style::Lime, Style::Shade500)] = "#CDDC39";
        colors[MaterialColor(Style::Lime, Style::Shade600)] = "#C0CA33";
        colors[MaterialColor(Style::Lime, Style::Shade700)] = "#AFB42B";
        colors[MaterialColor(Style::Lime, Style::Shade800)] = "#9E9D24";
        colors[MaterialColor(Style::Lime, Style::Shade900)] = "#827717";
        colors[MaterialColor(Style::Lime, Style::ShadeA100)] = "#F4FF81";
        colors[MaterialColor(Style::Lime, Style::ShadeA200)] = "#EEFF41";
        colors[MaterialColor(Style::Lime, Style::ShadeA400)] = "#C6FF00";
        colors[MaterialColor(Style::Lime, Style::ShadeA700)] = "#AEEA00";
        colors[MaterialColor(Style::Yellow, Style::Shade50)] = "#FFFDE7";
        colors[MaterialColor(Style::Yellow, Style::Shade100)] = "#FFF9C4";
        colors[MaterialColor(Style::Yellow, Style::Shade200)] = "#FFF59D";
        colors[MaterialColor(Style::Yellow, Style::Shade300)] = "#FFF176";
        colors[MaterialColor(Style::Yellow, Style::Shade400)] = "#FFEE58";
        colors[MaterialColor(Style::Yellow, Style::Shade500)] = "#FFEB3B";
        colors[MaterialColor(Style::Yellow, Style::Shade600)] = "#FDD835";
        colors[MaterialColor(Style::Yellow, Style::Shade700)] = "#FBC02D";
        colors[MaterialColor(Style::Yellow, Style::Shade800)] = "#F9A825";
        colors[MaterialColor(Style::Yellow, Style::Shade900)] = "#F57F17";
        colors[MaterialColor(Style::Yellow, Style::ShadeA100)] = "#FFFF8D";
        colors[MaterialColor(Style::Yellow, Style::ShadeA200)] = "#FFFF00";
        colors[MaterialColor(Style::Yellow, Style::ShadeA400)] = "#FFEA00";
        colors[MaterialColor(Style::Yellow, Style::ShadeA700)] = "#FFD600";
        colors[MaterialColor(Style::Amber, Style::Shade50)] = "#FFF8E1";
        colors[MaterialColor(Style::Amber, Style::Shade100)] = "#FFECB3";
        colors[MaterialColor(Style::Amber, Style::Shade200)] = "#FFE082";
        colors[MaterialColor(Style::Amber, Style::Shade300)] = "#FFD54F";
        colors[MaterialColor(Style::Amber, Style::Shade400)] = "#FFCA28";
        colors[MaterialColor(Style::Amber, Style::Shade500)] = "#FFC107";
        colors[MaterialColor(Style::Amber, Style::Shade600)] = "#FFB300";
        colors[MaterialColor(Style::Amber, Style::Shade700)] = "#FFA000";
        colors[MaterialColor(Style::Amber, Style::Shade800)] = "#FF8F00";
        colors[MaterialColor(Style::Amber, Style::Shade900)] = "#FF6F00";
        colors[MaterialColor(Style::Amber, Style::ShadeA100)] = "#FFE57F";
        colors[MaterialColor(Style::Amber, Style::ShadeA200)] = "#FFD740";
        colors[MaterialColor(Style::Amber, Style::ShadeA400)] = "#FFC400";
        colors[MaterialColor(Style::Amber, Style::ShadeA700)] = "#FFAB00";
        colors[MaterialColor(Style::Orange, Style::Shade50)] = "#FFF3E0";
        colors[MaterialColor(Style::Orange, Style::Shade100)] = "#FFE0B2";
        colors[MaterialColor(Style::Orange, Style::Shade200)] = "#FFCC80";
        colors[MaterialColor(Style::Orange, Style::Shade300)] = "#FFB74D";
        colors[MaterialColor(Style::Orange, Style::Shade400)] = "#FFA726";
        colors[MaterialColor(Style::Orange, Style::Shade500)] = "#FF9800";
        colors[MaterialColor(Style::Orange, Style::Shade600)] = "#FB8C00";
        colors[MaterialColor(Style::Orange, Style::Shade700)] = "#F57C00";
        colors[MaterialColor(Style::Orange, Style::Shade800)] = "#EF6C00";
        colors[MaterialColor(Style::Orange, Style::Shade900)] = "#E65100";
        colors[MaterialColor(Style::Orange, Style::ShadeA100)] = "#FFD180";
        colors[MaterialColor(Style::Orange, Style::ShadeA200)] = "#FFAB40";
        colors[MaterialColor(Style::Orange, Style::ShadeA400)] = "#FF9100";
        colors[MaterialColor(Style::Orange, Style::ShadeA700)] = "#FF6D00";
        colors[MaterialColor(Style::DeepOrange, Style::Shade50)] = "#FBE9E7";
        colors[MaterialColor(Style::DeepOrange, Style::Shade100)] = "#FFCCBC";
        colors[MaterialColor(Style::DeepOrange, Style::Shade200)] = "#FFAB91";
        colors[MaterialColor(Style::DeepOrange, Style::Shade300)] = "#FF8A65";
        colors[MaterialColor(Style::DeepOrange, Style::Shade400)] = "#FF7043";
        colors[MaterialColor(Style::DeepOrange, Style::Shade500)] = "#FF5722";
        colors[MaterialColor(Style::DeepOrange, Style::Shade600)] = "#F4511E";
        colors[MaterialColor(Style::DeepOrange, Style::Shade700)] = "#E64A19";
        colors[MaterialColor(Style::DeepOrange, Style::Shade800)] = "#D84315";
        colors[MaterialColor(Style::DeepOrange, Style::Shade900)] = "#BF360C";
        colors[MaterialColor(Style::DeepOrange, Style::ShadeA100)] = "#FF9E80";
        colors[MaterialColor(Style::DeepOrange, Style::ShadeA200)] = "#FF6E40";
        colors[MaterialColor(Style::DeepOrange, Style::ShadeA400)] = "#FF3D00";
        colors[MaterialColor(Style::DeepOrange, Style::ShadeA700)] = "#DD2C00";
        colors[MaterialColor(Style::Brown, Style::Shade50)] = "#EFEBE9";
        colors[MaterialColor(Style::Brown, Style::Shade100)] = "#D7CCC8";
        colors[MaterialColor(Style::Brown, Style::Shade200)] = "#BCAAA4";
        colors[MaterialColor(Style::Brown, Style::Shade300)] = "#A1887F";
        colors[MaterialColor(Style::Brown, Style::Shade400)] = "#8D6E63";
        colors[MaterialColor(Style::Brown, Style::Shade500)] = "#795548";
        colors[MaterialColor(Style::Brown, Style::Shade600)] = "#6D4C41";
        colors[MaterialColor(Style::Brown, Style::Shade700)] = "#5D4037";
        colors[MaterialColor(Style::Brown, Style::Shade800)] = "#4E342E";
        colors[MaterialColor(Style::Brown, Style::Shade900)] = "#3E2723";
        colors[MaterialColor(Style::Grey, Style::Shade50)] = "#FAFAFA";
        colors[MaterialColor(Style::Grey, Style::Shade100)] = "#F5F5F5";
        colors[MaterialColor(Style::Grey, Style::Shade200)] = "#EEEEEE";
        colors[MaterialColor(Style::Grey, Style::Shade300)] = "#E0E0E0";
        colors[MaterialColor(Style::Grey, Style::Shade400)] = "#BDBDBD";
        colors[MaterialColor(Style::Grey, Style::Shade500)] = "#9E9E9E";
        colors[MaterialColor(Style::Grey, Style::Shade600)] = "#757575";
        colors[MaterialColor(Style::Grey, Style::Shade700)] = "#616161";
        colors[MaterialColor(Style::Grey, Style::Shade800)] = "#424242";
        colors[MaterialColor(Style::Grey, Style::Shade900)] = "#212121";
        colors[MaterialColor(Style::BlueGrey, Style::Shade50)] = "#ECEFF1";
        colors[MaterialColor(Style::BlueGrey, Style::Shade100)] = "#CFD8DC";
        colors[MaterialColor(Style::BlueGrey, Style::Shade200)] = "#B0BEC5";
        colors[MaterialColor(Style::BlueGrey, Style::Shade300)] = "#90A4AE";
        colors[MaterialColor(Style::BlueGrey, Style::Shade400)] = "#78909C";
        colors[MaterialColor(Style::BlueGrey, Style::Shade500)] = "#607D8B";
        colors[MaterialColor(Style::BlueGrey, Style::Shade600)] = "#546E7A";
        colors[MaterialColor(Style::BlueGrey, Style::Shade700)] = "#455A64";
        colors[MaterialColor(Style::BlueGrey, Style::Shade800)] = "#37474F";
        colors[MaterialColor(Style::BlueGrey, Style::Shade900)] = "#263238";
    }
}

QQuickMaterialStyle::QQuickMaterialStyle(QObject *parent) :
    QObject(*(new QQuickMaterialStylePrivate()), parent)
{
    Q_D(QQuickMaterialStyle);
    QQuickItem *item = qobject_cast<QQuickItem *>(parent);
    if (item)
        QQuickItemPrivate::get(item)->addItemChangeListener(d, QQuickItemPrivate::Parent);
}

QQuickMaterialStyle::~QQuickMaterialStyle()
{
    Q_D(QQuickMaterialStyle);
    QQuickItem *item = qobject_cast<QQuickItem *>(parent());
    if (item)
        QQuickItemPrivate::get(item)->removeItemChangeListener(d, QQuickItemPrivate::Parent);

    reparent(Q_NULLPTR);
}

QQuickMaterialStyle *QQuickMaterialStyle::qmlAttachedProperties(QObject *object)
{
    QQuickMaterialStyle *style = new QQuickMaterialStyle(object);
    QQuickMaterialStyle *parent = QQuickStyle::findParent<QQuickMaterialStyle>(object);
    if (parent)
        style->reparent(parent);

    QList<QQuickMaterialStyle *> childStyles = QQuickStyle::findChildren<QQuickMaterialStyle>(object);
    foreach (QQuickMaterialStyle *child, childStyles)
        child->reparent(style);
    return style;
}

QQuickMaterialStyle::Theme QQuickMaterialStyle::theme() const
{
    Q_D(const QQuickMaterialStyle);
    return d->theme;
}

void QQuickMaterialStyle::setTheme(Theme theme)
{
    Q_D(QQuickMaterialStyle);
    d->explicitTheme = true;
    if (d->theme != theme) {
        d->theme = theme;
        foreach (QQuickMaterialStyle *child, d->childStyles) {
            child->inheritTheme(theme);
        }
        emit themeChanged();
        emit paletteChanged();
    }
}

void QQuickMaterialStyle::inheritTheme(Theme theme)
{
    Q_D(QQuickMaterialStyle);
    if (!d->explicitTheme && d->theme != theme) {
        d->theme = theme;
        foreach (QQuickMaterialStyle *child, d->childStyles) {
            child->inheritTheme(theme);
        }
        emit themeChanged();
        emit paletteChanged();
    }
}

void QQuickMaterialStyle::resetTheme()
{
    Q_D(QQuickMaterialStyle);
    if (d->explicitTheme) {
        d->explicitTheme = false;
        QQuickMaterialStyle *attachedParent = QQuickStyle::findParent<QQuickMaterialStyle>(parent());
        inheritTheme(attachedParent ? attachedParent->theme() : defaultTheme);
    }
}

QQuickMaterialStyle::Color QQuickMaterialStyle::primary() const
{
    Q_D(const QQuickMaterialStyle);
    return d->primary;
}

void QQuickMaterialStyle::setPrimary(QQuickMaterialStyle::Color color)
{
    Q_D(QQuickMaterialStyle);
    d->explicitPrimary = true;
    if (d->primary != color) {
        d->primary = color;
        emit primaryChanged();
        emit paletteChanged();

        foreach (QQuickMaterialStyle *child, d->childStyles)
            child->inheritPrimary(color);
    }
}

void QQuickMaterialStyle::inheritPrimary(QQuickMaterialStyle::Color color)
{
    Q_D(QQuickMaterialStyle);
    if (!d->explicitPrimary && d->primary != color) {
        d->primary = color;
        foreach (QQuickMaterialStyle *child, d->childStyles)
            child->inheritPrimary(color);
        emit primaryChanged();
    }
}

void QQuickMaterialStyle::resetPrimary()
{
    Q_D(QQuickMaterialStyle);
    if (d->explicitPrimary) {
        d->explicitPrimary = false;
        QQuickMaterialStyle *attachedParent = QQuickStyle::findParent<QQuickMaterialStyle>(parent());
        inheritPrimary(attachedParent ? attachedParent->primary() : defaultPrimary);
    }
}

QQuickMaterialStyle::Color QQuickMaterialStyle::accent() const
{
    Q_D(const QQuickMaterialStyle);
    return d->accent;
}

void QQuickMaterialStyle::setAccent(QQuickMaterialStyle::Color color)
{
    Q_D(QQuickMaterialStyle);
    d->explicitAccent = true;
    if (d->accent != color) {
        d->accent = color;
        emit accentChanged();
        emit paletteChanged();

        foreach (QQuickMaterialStyle *child, d->childStyles)
            child->inheritAccent(color);
    }
}

void QQuickMaterialStyle::inheritAccent(QQuickMaterialStyle::Color color)
{
    Q_D(QQuickMaterialStyle);
    if (!d->explicitAccent && d->accent != color) {
        d->accent = color;
        foreach (QQuickMaterialStyle *child, d->childStyles)
            child->inheritAccent(color);
        emit accentChanged();
    }
}

void QQuickMaterialStyle::resetAccent()
{
    Q_D(QQuickMaterialStyle);
    if (d->explicitAccent) {
        d->explicitAccent = false;
        QQuickMaterialStyle *attachedParent = QQuickStyle::findParent<QQuickMaterialStyle>(parent());
        inheritAccent(attachedParent ? attachedParent->accent() : defaultAccent);
    }
}

QColor QQuickMaterialStyle::accentColor() const
{
    Q_D(const QQuickMaterialStyle);
    return color(d->accent, Shade500);
}

QColor QQuickMaterialStyle::backgroundColor() const
{
    Q_D(const QQuickMaterialStyle);
    return d->theme == Light ? backgroundColorLight : backgroundColorDark;
}

QColor QQuickMaterialStyle::primaryTextColor() const
{
    Q_D(const QQuickMaterialStyle);
    return d->theme == Light ? primaryTextColorLight : primaryTextColorDark;
}

QColor QQuickMaterialStyle::primaryHighlightedTextColor() const
{
    return primaryTextColorDark;
}

QColor QQuickMaterialStyle::secondaryTextColor() const
{
    Q_D(const QQuickMaterialStyle);
    return d->theme == Light ? secondaryTextColorLight : secondaryTextColorDark;
}

QColor QQuickMaterialStyle::hintTextColor() const
{
    Q_D(const QQuickMaterialStyle);
    return d->theme == Light ? hintTextColorLight : hintTextColorDark;
}

QColor QQuickMaterialStyle::textSelectionColor() const
{
    QColor color = accentColor();
    color.setAlphaF(0.4);
    return color;
}

QColor QQuickMaterialStyle::dropShadowColor() const
{
    return QColor("#40000000");
}

QColor QQuickMaterialStyle::dividerColor() const
{
    Q_D(const QQuickMaterialStyle);
    return d->theme == Light ? dividerTextColorLight : dividerTextColorDark;
}

QColor QQuickMaterialStyle::raisedButtonColor() const
{
    Q_D(const QQuickMaterialStyle);
    return d->theme == Light ? raisedButtonColorLight : flatButtonFocusColorDark;
}

QColor QQuickMaterialStyle::raisedButtonHoverColor() const
{
    Q_D(const QQuickMaterialStyle);
    // The specs don't specify different colors here for the light theme.
    return d->theme == Light ? raisedButtonColorLight : flatButtonPressColorDark;
}

QColor QQuickMaterialStyle::raisedButtonPressColor() const
{
    Q_D(const QQuickMaterialStyle);
    return d->theme == Light ? raisedButtonPressColorLight : flatButtonPressColorDark;
}

QColor QQuickMaterialStyle::raisedButtonDisabledColor() const
{
    Q_D(const QQuickMaterialStyle);
    return d->theme == Light ? raisedButtonDisabledColorLight : raisedButtonDisabledColorDark;
}

QColor QQuickMaterialStyle::raisedHighlightedButtonColor() const
{
    return accentColor();
}

QColor QQuickMaterialStyle::raisedHighlightedButtonHoverColor() const
{
    Q_D(const QQuickMaterialStyle);
    return color(d->accent, Shade600);
}

QColor QQuickMaterialStyle::raisedHighlightedButtonPressColor() const
{
    Q_D(const QQuickMaterialStyle);
    return color(d->accent, Shade700);
}

QColor QQuickMaterialStyle::raisedHighlightedButtonDisabledColor() const
{
    Q_D(const QQuickMaterialStyle);
    return d->theme == Light ? raisedButtonDisabledColorLight : raisedButtonDisabledColorDark;
}

QColor QQuickMaterialStyle::flatButtonPressColor() const
{
    Q_D(const QQuickMaterialStyle);
    return d->theme == Light ? flatButtonPressColorLight : flatButtonPressColorDark;
}

QColor QQuickMaterialStyle::flatButtonFocusColor() const
{
    Q_D(const QQuickMaterialStyle);
    return d->theme == Light ? flatButtonFocusColorLight : flatButtonFocusColorDark;
}

QColor QQuickMaterialStyle::frameColor() const
{
    Q_D(const QQuickMaterialStyle);
    return d->theme == Light ? frameColorLight : frameColorDark;
}

QColor QQuickMaterialStyle::checkBoxUncheckedRippleColor() const
{
    Q_D(const QQuickMaterialStyle);
    return d->theme == Light ? checkBoxUncheckedRippleColorLight : checkBoxUncheckedRippleColorDark;
}

QColor QQuickMaterialStyle::checkBoxCheckedRippleColor() const
{
    Q_D(const QQuickMaterialStyle);
    QColor pressColor = color(d->accent, Shade500);
    // TODO: find out actual value
    pressColor.setAlpha(30);
    return pressColor;
}

QColor QQuickMaterialStyle::switchUncheckedTrackColor() const
{
    Q_D(const QQuickMaterialStyle);
    return d->theme == Light ? switchUncheckedTrackColorLight : switchUncheckedTrackColorDark;
}

QColor QQuickMaterialStyle::switchCheckedTrackColor() const
{
    Q_D(const QQuickMaterialStyle);
    QColor trackColor = d->theme == Light ? accentColor() : color(d->accent, Shade200);
    trackColor.setAlphaF(0.5);
    return trackColor;
}

QColor QQuickMaterialStyle::switchUncheckedHandleColor() const
{
    Q_D(const QQuickMaterialStyle);
    return d->theme == Light ? color(Grey, Shade50) : color(Grey, Shade400);
}

QColor QQuickMaterialStyle::switchCheckedHandleColor() const
{
    Q_D(const QQuickMaterialStyle);
    return d->theme == Light ? accentColor() : color(d->accent, Shade200);
}

QColor QQuickMaterialStyle::switchDisabledTrackColor() const
{
    Q_D(const QQuickMaterialStyle);
    return d->theme == Light ? switchDisabledTrackColorLight : switchDisabledTrackColorDark;
}

QColor QQuickMaterialStyle::switchDisabledHandleColor() const
{
    Q_D(const QQuickMaterialStyle);
    return d->theme == Light ? color(Grey, Shade400) : color(Grey, Shade800);
}

QColor QQuickMaterialStyle::scrollBarColor() const
{
    Q_D(const QQuickMaterialStyle);
    return d->theme == Light ? "#40000000" : "#40FFFFFF";
}

QColor QQuickMaterialStyle::scrollBarPressedColor() const
{
    Q_D(const QQuickMaterialStyle);
    return d->theme == Light ? "#80000000" : "#80FFFFFF";
}

QColor QQuickMaterialStyle::drawerBackgroundColor() const
{
    return dividerTextColorLight;
}

QColor QQuickMaterialStyle::color(QQuickMaterialStyle::Color color, QQuickMaterialStyle::Shade shade) const
{
    Q_D(const QQuickMaterialStyle);
    return d->colors.value(MaterialColor(color, shade));
}

void QQuickMaterialStyle::reparent(QQuickMaterialStyle *style)
{
    Q_D(QQuickMaterialStyle);
    if (d->parentStyle != style) {
        if (d->parentStyle)
            d->parentStyle->d_func()->childStyles.remove(this);
        d->parentStyle = style;
        if (style) {
            style->d_func()->childStyles.insert(this);
            inheritPrimary(style->primary());
            inheritAccent(style->accent());
            inheritTheme(style->theme());
        }
    }
}

void QQuickMaterialStyle::itemParentChanged(QQuickItem *item, QQuickItem *parentItem)
{
    QQuickMaterialStyle *style = QQuickStyle::instance<QQuickMaterialStyle>(item);
    if (style) {
        QQuickMaterialStyle *parent = QQuickStyle::findParent<QQuickMaterialStyle>(parentItem);
        if (parent)
            style->reparent(parent);
    }
}

QT_END_NAMESPACE
