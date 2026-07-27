pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.discover.app as DiscoverApp
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD

DiscoverPage {
    id: page

    property string search
    readonly property string name: title

    clip: true
    title: i18n("Settings")

    header: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        Repeater {
            model: Discover.SourcesModel.sources

            delegate: Kirigami.InlineMessage {
                id: delegate

                required property Discover.AbstractSourcesBackend modelData

                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.smallSpacing
                text: modelData.inlineAction?.toolTip ?? ""
                visible: modelData.inlineAction?.visible ?? false
                actions: Kirigami.Action {
                    icon.name: delegate.modelData.inlineAction?.iconName ?? ""
                    text: delegate.modelData.inlineAction?.text ?? ""
                    onTriggered: delegate.modelData.inlineAction?.trigger()
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Kirigami.Units.largeSpacing

        ListView {
            id: sourcesView

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: Kirigami.Units.gridUnit * 8

            model: Discover.SourcesModel
            Component.onCompleted: Qt.callLater(Discover.SourcesModel.showingNow)
            currentIndex: -1
            pixelAligned: true
            clip: true
            QQC2.ScrollBar.vertical: QQC2.ScrollBar {}
            section.property: "sourceName"
            section.delegate: Kirigami.ListSectionHeader {
            id: backendItem

            required property string section

            height: Math.ceil(Math.max(Kirigami.Units.gridUnit * 2.5, contentItem.implicitHeight))

            readonly property Discover.AbstractSourcesBackend backend: Discover.SourcesModel.sourcesBackendByName(section)
            readonly property Discover.AbstractResourcesBackend resourcesBackend: backend.resourcesBackend
            readonly property bool isDefault: Discover.ResourcesModel.currentApplicationBackend === resourcesBackend

            width: sourcesView.width

            Connections {
                target: backendItem.backend
                function onPassiveMessage(message) {
                    window.showPassiveNotification(message)
                }
                function onProceedRequest(title, description) {
                    const dialog = sourceProceedDialog.createObject(window, {
                        sourcesBackend: backendItem.backend,
                        title,
                        description,
                    })
                    dialog.open()
                }
            }

            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Heading {
                    text: resourcesBackend.displayName
                    level: 3
                    font.weight: backendItem.isDefault ? Font.Bold : Font.Normal
                }

                Kirigami.ActionToolBar {
                    id: actionBar

                    alignment: Qt.AlignRight

                    Kirigami.Action {
                        id: addSourceAction
                        text: i18n("Add Source…")
                        icon.name: "list-add"
                        visible: backendItem.backend && backendItem.backend.supportsAdding

                        onTriggered: {
                            const addSourceDialog = dialogComponent.createObject(window, {
                                displayName: backendItem.backend.resourcesBackend.displayName,
                            })
                            addSourceDialog.open()
                        }
                    }

                    Component {
                        id: dialogComponent
                        AddSourceDialog {
                            source: backendItem.backend

                            onClosed: {
                                destroy();
                            }
                        }
                    }

                    Kirigami.Action {
                        id: makeDefaultAction
                        visible: resourcesBackend && resourcesBackend.hasApplications && !backendItem.isDefault

                        text: i18n("Make Default")
                        icon.name: "favorite"
                        onTriggered: Discover.ResourcesModel.currentApplicationBackend = backendItem.backend.resourcesBackend
                    }

                    Component {
                        id: kirigamiAction
                        ConvertDiscoverAction {}
                    }

                    function mergeActions(moreActions) {
                        const actions = [
                            makeDefaultAction,
                            addSourceAction
                        ]
                        for (const action of moreActions) {
                            actions.push(kirigamiAction.createObject(this, { action }))
                        }
                        return actions;
                    }
                    actions: mergeActions(backendItem.backend.actions)
                }
            }
        }

        Component {
            id: sourceProceedDialog
            Kirigami.OverlaySheet {
                id: sheet

                property Discover.AbstractSourcesBackend sourcesBackend
                property alias description: descriptionLabel.text
                property bool acted: false

                parent: page.QQC2.Overlay.overlay
                showCloseButton: false

                implicitWidth: Kirigami.Units.gridUnit * 30

                Kirigami.SelectableLabel {
                    id: descriptionLabel
                    width: parent.width
                    textFormat: TextEdit.RichText
                    wrapMode: TextEdit.Wrap
                }

                footer: QQC2.DialogButtonBox {
                    QQC2.Button {
                        QQC2.DialogButtonBox.buttonRole: QQC2.DialogButtonBox.AcceptRole
                        text: i18n("Proceed")
                        icon.name: "dialog-ok"
                    }

                    QQC2.Button {
                        QQC2.DialogButtonBox.buttonRole: QQC2.DialogButtonBox.RejectRole
                        text: i18n("Cancel")
                        icon.name: "dialog-cancel"
                    }

                    onAccepted: {
                        sheet.sourcesBackend.proceed()
                        sheet.acted = true
                        sheet.close()
                    }

                    onRejected: {
                        sheet.sourcesBackend.cancel()
                        sheet.acted = true
                        sheet.close()
                    }
                }

                onOpened: {
                    descriptionLabel.forceActiveFocus(Qt.PopupFocusReason);
                }

                onClosed: {
                    if (!acted) {
                        sourcesBackend.cancel()
                    }
                    destroy();
                }
            }
        }

        delegate: Kirigami.SwipeListItem {
            id: delegate

            required property int index
            required property var model

            enabled: model.display.length > 0 && model.enabled
            highlighted: ListView.isCurrentItem
            supportsMouseEvents: false
            visible: model.display.indexOf(page.search) >= 0
            height: visible ? implicitHeight : 0

            Keys.onReturnPressed: enabledBox.clicked()
            Keys.onSpacePressed: enabledBox.clicked()
            actions: [
                Kirigami.Action {
                    icon.name: "go-up"
                    tooltip: i18n("Increase priority")
                    enabled: delegate.model.sourcesBackend.firstDisambiguatedSourceId !== delegate.model.disambiguatedSourceId
                    visible: delegate.model.sourcesBackend.canMoveSources
                    onTriggered: {
                        const ret = delegate.model.sourcesBackend.moveSource(delegate.model.disambiguatedSourceId, -1)
                        if (!ret) {
                            window.showPassiveNotification(i18n("Failed to increase '%1' preference", delegate.model.display))
                        }
                    }
                },
                Kirigami.Action {
                    icon.name: "go-down"
                    tooltip: i18n("Decrease priority")
                    enabled: delegate.model.sourcesBackend.lastDisambiguatedSourceId !== delegate.model.disambiguatedSourceId
                    visible: delegate.model.sourcesBackend.canMoveSources
                    onTriggered: {
                        const ret = delegate.model.sourcesBackend.moveSource(delegate.model.disambiguatedSourceId, +1)
                        if (!ret) {
                            window.showPassiveNotification(i18n("Failed to decrease '%1' preference", delegate.model.display))
                        }
                    }
                },
                Kirigami.Action {
                    icon.name: "edit-delete"
                    tooltip: i18n("Remove repository")
                    visible: delegate.model.sourcesBackend.supportsAdding
                    onTriggered: {
                        const backend = delegate.model.sourcesBackend
                        if (!backend.removeSource(delegate.model.disambiguatedSourceId)) {
                            console.warn("Failed to remove the source", delegate.model.display)
                        }
                    }
                },
                Kirigami.Action {
                    icon.name: delegate.mirrored ? "go-next-symbolic-rtl" : "go-next-symbolic"
                    tooltip: i18n("Show contents")
                    visible: delegate.model.sourcesBackend.canFilterSources
                    onTriggered: {
                        Navigation.openApplicationListSource(delegate.model.disambiguatedSourceId, delegate.model.display)
                    }
                }
            ]

            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                QQC2.CheckBox {
                    id: enabledBox

                    readonly property var idx: index !== -1 ? sourcesView.model.index(index, 0) : null
                    readonly property /*Qt::CheckState*/int modelChecked: delegate.model.checkState
                    checked: modelChecked !== Qt.Unchecked
                    enabled: idx && sourcesView.model.flags(idx) & Qt.ItemIsUserCheckable
                    onClicked: if (idx) {
                        sourcesView.model.setData(idx, checkState, Qt.CheckStateRole)
                        checked = Qt.binding(() => (modelChecked !== Qt.Unchecked))
                    }
                    QQC2.ToolTip.text: i18nc("@info:tooltip", "Enable this source")
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                }
                QQC2.Label {
                    text: delegate.model.display + (delegate.model.toolTip ? " - <i>" + delegate.model.toolTip + "</i>" : "")
                    elide: Text.ElideRight
                    textFormat: Text.StyledText
                    Layout.fillWidth: true
                }
            }
        }

            footer: ColumnLayout {
                spacing: 0
                width: ListView.view.width

            Kirigami.ListSectionHeader {
                Layout.fillWidth: true

                visible: back.count > 0
                text: i18n("Missing Backends")
            }

            Repeater {
                id: back
                model: Discover.ResourcesProxyModel {
                    extending: "org.kde.discover.desktop"
                    filterMinimumState: false
                    stateFilter: Discover.AbstractResource.None
                }
                delegate: QQC2.ItemDelegate {
                    id: delegate

                    required property int index
                    required property var model
                    required property string name

                    Layout.fillWidth: true
                    background: null
                    hoverEnabled: false
                    down: false

                    contentItem: RowLayout {
                        spacing: Kirigami.Units.smallSpacing
                        KD.IconTitleSubtitle {
                            title: name
                            icon.source: delegate.model.icon
                            subtitle: delegate.model.comment
                            Layout.fillWidth: true
                        }
                        InstallApplicationButton {
                            application: delegate.model.application
                        }
                    }
                }
            }
            }
        }

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        Kirigami.FormLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            Layout.bottomMargin: Kirigami.Units.largeSpacing

            QQC2.ButtonGroup {
                id: automaticUpdatesGroup
            }

            QQC2.RadioButton {
                Kirigami.FormData.label: i18n("App updates")
                text: i18nd("kcm_updates", "Manually")
                QQC2.ButtonGroup.group: automaticUpdatesGroup
                checked: !DiscoverApp.AppUpdateSettings.automaticUpdates
                onClicked: DiscoverApp.AppUpdateSettings.automaticUpdates = false
            }

            RowLayout {
                QQC2.RadioButton {
                    text: i18nd("kcm_updates", "Automatically")
                    QQC2.ButtonGroup.group: automaticUpdatesGroup
                    checked: DiscoverApp.AppUpdateSettings.automaticUpdates
                    onClicked: {
                        if (DiscoverApp.AppUpdateSettings.updateInterval <= 0) {
                            DiscoverApp.AppUpdateSettings.updateInterval = 24 * 60 * 60
                        }
                        DiscoverApp.AppUpdateSettings.automaticUpdates = true
                    }
                }
            }

            QQC2.ComboBox {
                Kirigami.FormData.label: DiscoverApp.AppUpdateSettings.automaticUpdates
                    ? i18ndc("kcm_updates", "@title:group", "Update frequency:")
                    : i18ndc("kcm_updates", "@title:group", "Notification frequency:")

                readonly property var frequencyOptions: [
                    24 * 60 * 60,
                    7 * 24 * 60 * 60,
                    30 * 24 * 60 * 60,
                    -1
                ]
                readonly property var frequencyLabels: [
                    i18ndc("kcm_updates", "@item:inlistbox", "Daily"),
                    i18ndc("kcm_updates", "@item:inlistbox", "Weekly"),
                    i18ndc("kcm_updates", "@item:inlistbox", "Monthly"),
                    i18ndc("kcm_updates", "@item:inlistbox", "Never")
                ]

                model: DiscoverApp.AppUpdateSettings.automaticUpdates
                    ? frequencyLabels.slice(0, 3)
                    : frequencyLabels
                currentIndex: Math.max(0, frequencyOptions.indexOf(DiscoverApp.AppUpdateSettings.updateInterval))
                onActivated: index => {
                    DiscoverApp.AppUpdateSettings.updateInterval = frequencyOptions[index]
                }
            }
        }
    }
}
