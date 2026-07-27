/*
 *   SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2021 Carl Schwan <carl@carlschwan.eu>
 *   SPDX-FileCopyrightText: 2026 FluffNet LLC
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.coreaddons as Core
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as KirigamiComponents
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardPage {
    id: page

    readonly property bool isHome: true
    readonly property var aboutData: Core.AboutData
    readonly property var copyrightLines: aboutData.copyrightStatement.split("\n")
    readonly property var libraries: FormCard.AboutComponent.components.filter(component => {
        return component.description !== i18nd("kirigami-addons6", "Distribution method.");
    })

    title: i18nd("kirigami-addons6", "About %1", aboutData.displayName)

    FormCard.FormCard {
        Layout.topMargin: Kirigami.Units.largeSpacing * 4

        FormCard.AbstractFormDelegate {
            Layout.fillWidth: true
            background: null

            contentItem: RowLayout {
                spacing: Kirigami.Units.largeSpacing

                Kirigami.Icon {
                    Layout.preferredHeight: Kirigami.Units.iconSizes.huge
                    Layout.preferredWidth: height
                    Layout.maximumWidth: page.width / 3
                    source: page.aboutData.programLogo || Kirigami.Settings.applicationWindowIcon || page.aboutData.componentName
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Kirigami.Heading {
                        Layout.fillWidth: true
                        text: page.aboutData.displayName + " " + page.aboutData.version
                        wrapMode: Text.WordWrap
                    }

                    Kirigami.Heading {
                        Layout.fillWidth: true
                        level: 3
                        type: Kirigami.Heading.Type.Secondary
                        text: page.aboutData.shortDescription
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }

        FormCard.FormDelegateSeparator {}

        FormCard.AbstractFormDelegate {
            Layout.fillWidth: true
            background: null

            contentItem: ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                QQC2.Label {
                    Layout.fillWidth: true
                    text: i18nd("kirigami-addons6", "Copyright")
                    font.bold: true
                }

                QQC2.Label {
                    Layout.fillWidth: true
                    text: page.copyrightLines[0]
                    textFormat: Text.PlainText
                    wrapMode: Text.WordWrap
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: page.copyrightLines[1]
                        textFormat: Text.PlainText
                        wrapMode: Text.WordWrap
                    }

                    QQC2.ToolButton {
                        text: i18n("Original Authors")
                        icon.name: "information-symbolic"
                        display: QQC2.AbstractButton.IconOnly
                        onClicked: authorsDialog.open()

                        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: text

                        Accessible.name: text
                    }
                }
            }
        }
    }

    FormCard.FormCard {
        Layout.topMargin: Kirigami.Units.gridUnit
        visible: page.aboutData.bugAddress.length > 0

        FormCard.FormLinkDelegate {
            icon.name: "tools-report-bug-symbolic"
            text: i18nd("kirigami-addons6", "Report a Bug")
            url: page.aboutData.bugAddress
        }
    }

    FormCard.FormHeader {
        title: i18nd("kirigami-addons6", "Libraries in use")
    }

    FormCard.FormCard {
        Repeater {
            model: page.libraries

            delegate: FormCard.AbstractFormDelegate {
                required property var modelData

                Layout.fillWidth: true
                background: null

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: modelData.name + " " + modelData.version
                        wrapMode: Text.WordWrap
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        visible: text.length > 0
                        text: modelData.description
                        color: Kirigami.Theme.disabledTextColor
                        font: Kirigami.Theme.smallFont
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
    }

    FormCard.FormHeader {
        visible: page.aboutData.credits.length > 0
        title: i18nd("kirigami-addons6", "Credits")
    }

    FormCard.FormCard {
        visible: page.aboutData.credits.length > 0

        Repeater {
            model: page.aboutData.credits
            delegate: contributorDelegate
        }
    }

    FormCard.FormHeader {
        visible: page.aboutData.translators.length > 0
        title: i18nd("kirigami-addons6", "Translators")
    }

    FormCard.FormCard {
        visible: page.aboutData.translators.length > 0

        Repeater {
            model: page.aboutData.translators
            delegate: contributorDelegate
        }
    }

    data: [
        Component {
            id: contributorDelegate

            FormCard.AbstractFormDelegate {
                Layout.fillWidth: true
                background: null

                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: modelData.name
                        elide: Text.ElideRight
                    }

                    QQC2.Label {
                        Layout.fillWidth: true
                        visible: text.length > 0
                        text: modelData.task
                        color: Kirigami.Theme.disabledTextColor
                        font: Kirigami.Theme.smallFont
                        wrapMode: Text.WordWrap
                    }
                }
            }
        },

        KirigamiComponents.MessageDialog {
            id: authorsDialog

            parent: page.QQC2.Overlay.overlay
            title: i18n("Original Authors")

            leftPadding: 0
            rightPadding: 0
            bottomPadding: 0
            topPadding: 0

            header: QQC2.Control {
                padding: authorsDialog.padding
                topPadding: Kirigami.Units.largeSpacing
                bottomPadding: Kirigami.Units.largeSpacing

                contentItem: RowLayout {
                    spacing: Kirigami.Units.largeSpacing

                    Kirigami.Heading {
                        Layout.fillWidth: true
                        text: authorsDialog.title
                        elide: Text.ElideRight
                    }

                    QQC2.ToolButton {
                        icon.name: hovered ? "window-close" : "window-close-symbolic"
                        text: i18ndc("kirigami-addons6", "@action:button", "Close")
                        display: QQC2.AbstractButton.IconOnly
                        onClicked: authorsDialog.close()

                        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                        QQC2.ToolTip.visible: hovered
                        QQC2.ToolTip.text: text
                    }
                }

                Kirigami.Separator {
                    anchors {
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                    }
                }
            }

            contentItem: QQC2.ScrollView {
                implicitWidth: Kirigami.Units.gridUnit * 24
                implicitHeight: Math.min(authorsColumn.implicitHeight, Kirigami.Units.gridUnit * 20)

                ColumnLayout {
                    id: authorsColumn
                    width: parent.width
                    spacing: 0

                    Repeater {
                        model: page.aboutData.authors

                        delegate: FormCard.AbstractFormDelegate {
                            required property var modelData

                            Layout.fillWidth: true
                            background: null

                            contentItem: ColumnLayout {
                                spacing: Kirigami.Units.smallSpacing

                                QQC2.Label {
                                    Layout.fillWidth: true
                                    text: modelData.name
                                    elide: Text.ElideRight
                                }

                                QQC2.Label {
                                    Layout.fillWidth: true
                                    visible: text.length > 0
                                    text: modelData.task
                                    color: Kirigami.Theme.disabledTextColor
                                    font: Kirigami.Theme.smallFont
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
                }
            }

            footer: null
        }
    ]
}
