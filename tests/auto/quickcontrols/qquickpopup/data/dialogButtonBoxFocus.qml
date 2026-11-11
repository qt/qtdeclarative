// Copyright (C) 2026 Kai Uwe Broulik <kde@broulik.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    Dialog {
        objectName: "defaultStandardButtonDialog"
        footer: DialogButtonBox {
            standardButtons: DialogButtonBox.Ok | DialogButtonBox.Yes | DialogButtonBox.No | DialogButtonBox.Cancel
            defaultStandardButton: DialogButtonBox.Cancel
        }
    }

    Dialog {
        objectName: "defaultButtonDialog"
        footer: DialogButtonBox {
            defaultButton: bazButton

            Button {
                text: "Foo"
            }
            Button {
                objectName: "acceptRoleButton"
                text: "Bar"
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
            Button {
                id: bazButton
                objectName: "defaultButton"
                text: "Baz"
            }
        }
    }

    Dialog {
        objectName: "acceptRoleDialog"
        footer: DialogButtonBox {
            Button {
                text: "Foo"
            }
            Button {
                objectName: "acceptRoleButton"
                text: "Bar"
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            }
        }
    }

    Dialog {
        objectName: "standardAcceptButtonDialog"
        footer: DialogButtonBox {
            standardButtons: DialogButtonBox.Ok | DialogButtonBox.Cancel
        }
    }

    Dialog {
        objectName: "noDefaultButtonDialog"
        footer: DialogButtonBox {
            Button {
                text: "Foo"
            }
            Button {
                text: "Bar"
            }
        }
    }
}
