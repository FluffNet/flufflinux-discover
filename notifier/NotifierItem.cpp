/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "NotifierItem.h"
#include "updatessettings.h"
#include <KLocalizedString>
#include <QMenu>

NotifierItem::NotifierItem(const std::chrono::seconds &checkDelay)
    : m_notifier(checkDelay)
{
    connect(&m_notifier, &DiscoverNotifier::stateChanged, this, &NotifierItem::refreshStatusNotifierVisibility);
}

void NotifierItem::setupNotifierItem()
{
    Q_ASSERT(!m_item);
    m_item = new KStatusNotifierItem(QStringLiteral("org.kde.DiscoverNotifier"), this);
    m_item->setTitle(i18n("App updates"));
    m_item->setToolTipTitle(i18n("App updates"));

    connect(m_item, &KStatusNotifierItem::activateRequested, &m_notifier, [this]() {
        m_notifier.showDiscoverUpdates(m_item->providedToken());
    });

    QMenu *menu = new QMenu;
    connect(m_item, &QObject::destroyed, menu, &QObject::deleteLater);
    auto discoverAction = menu->addAction(QIcon::fromTheme(QStringLiteral("flufflinuxplasmadiscover")),
                                          i18nc("@action:button Opens Discover's main UI to analyze the updates", "Open Discover…"));
    connect(discoverAction, &QAction::triggered, &m_notifier, [this] {
        // If there's updates open directly on the updates page, otherwise show the main page
        if (m_notifier.hasUpdates()) {
            m_notifier.showDiscoverUpdates(m_item->providedToken());
        } else {
            m_notifier.showDiscover(m_item->providedToken());
        }
    });

    auto updatesAction =
        menu->addAction(QIcon::fromTheme(QStringLiteral("system-software-update")), i18nc("@action:button Starts an update in the background", "Start Update"));
    connect(updatesAction, &QAction::triggered, &m_notifier, &DiscoverNotifier::startUnattendedUpdates);

    auto refreshAction = menu->addAction(QIcon::fromTheme(QStringLiteral("view-refresh")), i18n("Refresh…"));
    connect(refreshAction, &QAction::triggered, &m_notifier, &DiscoverNotifier::recheckSystemUpdateNeededAndNotifyApp);

    connect(&m_notifier, &DiscoverNotifier::newUpgradeAction, menu, [menu](UpgradeAction *a) {
        QAction *action = new QAction(a->description(), menu);
        connect(action, &QAction::triggered, a, &UpgradeAction::trigger);
        menu->addAction(action);
    });
    m_item->setContextMenu(menu);
    m_item->setStatus(KStatusNotifierItem::Active);
    refresh();
}

void NotifierItem::refreshStatusNotifierVisibility()
{
    bool shouldShow = shouldShowStatusNotifier();
    if (!m_item && shouldShow) {
        setStatusNotifierVisibility(true);
    } else if (m_item && !shouldShow) {
        setStatusNotifierVisibility(false);
    }
    refresh();
}

void NotifierItem::setStatusNotifierEnabled(bool enabled)
{
    m_statusNotifierEnabled = enabled;
    refreshStatusNotifierVisibility();
}

void NotifierItem::refresh()
{
    if (!m_item) {
        return;
    }
    m_item->setIconByName(m_notifier.iconName());
    m_item->setToolTipSubTitle(m_notifier.message());
}

void NotifierItem::setStatusNotifierVisibility(bool visible)
{
    if (visible) {
        Q_ASSERT(!m_item);
        setupNotifierItem();
    } else {
        Q_ASSERT(m_item);
        delete m_item;
    }
}

bool NotifierItem::shouldShowStatusNotifier() const
{
    if (!isStatusNotifierEnabled()) {
        return false;
    }

    // Only show the status notifier if there is something to notify about
    // BUG: 413053
    switch (m_notifier.state()) {
    case DiscoverNotifier::Busy:
        return true;
    case DiscoverNotifier::NormalUpdates: {
        // Only show the status notifier on next notification time
        // BUG: 466693
        const int interval = m_notifier.updateIntervalSeconds();
        const QDateTime earliestNextNotificationTime = m_notifier.lastNotificationTime().addSecs(interval);

        return !(earliestNextNotificationTime.isValid() && earliestNextNotificationTime > QDateTime::currentDateTimeUtc());
    }
    case DiscoverNotifier::Offline:
    case DiscoverNotifier::NoUpdates:
    default:
        return false;
    }
}

#include "moc_NotifierItem.cpp"
