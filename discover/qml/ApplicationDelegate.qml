/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2018-2026 Nate Graham <nate@kde.org>
 *   SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.kirigami as Kirigami

BasicAbstractCard {
    id: root

    required property int index
    required property Discover.AbstractResource application

    property bool showSize: false
    property bool showInstallButton: !compact

    readonly property bool compact: !applicationWindow().wideScreen
    readonly property int appIconSize: Kirigami.Units.iconSizes.large
    readonly property bool appIsFromNonDefaultBackend: Discover.ResourcesModel.currentApplicationBackend !== application.backend && application.backend.hasApplications
    readonly property int nonDefaultBackendLogoSize: Kirigami.Units.iconSizes.smallMedium
    readonly property int maximumLineCount: compact ? 3 : 4

    showClickFeedback: true
    activeFocusOnTab: true
    highlighted: focus

    Accessible.name: application.name
    Accessible.role: Accessible.ListItem
    Accessible.onPressAction: trigger()
    onClicked: trigger()

    function trigger() {
        ListView.currentIndex = index
        Navigation.openApplication(application)
    }

    QQC2.ToolTip.text: "<b>" + appName.text + "</b><br/>" + appDescription.text
    QQC2.ToolTip.visible: (hovered || activeFocus) && (appName.truncated || appDescription.truncated)
    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

    content: RowLayout {
        spacing: Kirigami.Units.largeSpacing

        // Container to center the app icon nicely
        // Also includes non-default backend badge and app size, if shown
        Item {
            Layout.fillHeight: true
            Layout.preferredWidth: height

            // Container for app icon and size (if visible)
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 0

                // App Icon
                Kirigami.Icon {
                    implicitWidth: root.appIconSize
                    implicitHeight: root.appIconSize
                    source: root.application.icon
                    animated: false
                }

                // App size
                Loader {
                    Layout.alignment: Qt.AlignHCenter
                    active: root.showSize
                    visible: active
                    sourceComponent: QQC2.Label {
                        text: root.application.sizeDescription
                        opacity: 0.75
                        font: Kirigami.Theme.smallFont
                        elide: Text.ElideRight
                        maximumLineCount: 1
                        textFormat: Text.PlainText
                    }
                }
            }

            // "Non-default backend" badge
            Loader {
                anchors.bottom: parent.bottom
                anchors.right: parent.right
                active: root.appIsFromNonDefaultBackend
                visible: active
                sourceComponent: Kirigami.Badge {
                    padding: 0
                    icon.width: root.nonDefaultBackendLogoSize
                    icon.height: root.nonDefaultBackendLogoSize
                    icon.source: root.application.sourceIcon
                    customColor: "white" // Backend logos aren't color-scheme-aware

                    QQC2.ToolTip.text: root.application.backend.displayName
                    QQC2.ToolTip.visible: hovered || activeFocus
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                }
            }
        }

        // Container for app name and description
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: (Kirigami.Units.gridUnit * root.maximumLineCount) + (appName.lineCount * Kirigami.Units.smallSpacing)
            spacing: 0

            // App Name
            Kirigami.Heading {
                id: appName
                level: 2
                type: Kirigami.Heading.Type.Primary
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignBottom
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight

                text: root.application.name
            }

            // App Description
            QQC2.Label {
                id: appDescription
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                maximumLineCount: root.maximumLineCount - appName.lineCount
                visible: maximumLineCount > 0
                opacity: 0.75
                wrapMode: Text.Wrap
                elide: Text.ElideRight
                textFormat: Text.PlainText

                text: root.application.comment
            }
        }

        // Install button
        Loader {
            active: root.showInstallButton
            visible: active
            Layout.rightMargin: Math.round((root.height - root.topPadding - root.leftPadding - root.bottomPadding - height) / 2)
            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            sourceComponent: InstallApplicationButton {
                application: root.application
                installOrRemoveButtonDisplayStyle: QQC2.AbstractButton.IconOnly
            }
        }
    }

    onFocusChanged: {
        if (focus) {
            page.ensureVisible(root)
        }
    }
}
