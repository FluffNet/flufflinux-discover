import QtQuick
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.discover.app as DiscoverApp

ApplicationsListPage {
    id: page

    stateFilter: Discover.AbstractResource.Installed
    allBackends: true
    sortProperty: "installedPageSorting"
    sortRole: DiscoverApp.DiscoverSettings.installedPageSorting

    name: i18n("Installed")
    showSize: true
    canNavigate: false
    canCategorize: true

    listHeader: null
}
