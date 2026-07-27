/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AppUpdateSettings.h"

#include <KConfigGroup>

using namespace Qt::Literals;

namespace
{
constexpr auto configFile = "PlasmaDiscoverUpdates";
constexpr auto configGroup = "Global";
constexpr auto automaticUpdatesKey = "UseUnattendedUpdates";
constexpr auto updateIntervalKey = "RequiredNotificationInterval";
}

AppUpdateSettings::AppUpdateSettings(QObject *parent)
    : QObject(parent)
    , m_config(KSharedConfig::openConfig(QLatin1String(configFile)))
    , m_watcher(KConfigWatcher::create(m_config))
{
    KConfigGroup group = m_config->group(QLatin1String(configGroup));
    bool createdDefaults = false;
    if (!group.hasKey(automaticUpdatesKey)) {
        group.writeEntry(automaticUpdatesKey, true);
        createdDefaults = true;
    }
    if (!group.hasKey(updateIntervalKey)) {
        group.writeEntry(updateIntervalKey, 7 * 24 * 60 * 60);
        createdDefaults = true;
    }
    if (createdDefaults) {
        m_config->sync();
    }

    reload();
    connect(m_watcher.data(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &group, const QByteArrayList &names) {
        if (group.name() == QLatin1String(configGroup)
            && (names.contains(automaticUpdatesKey) || names.contains(updateIntervalKey))) {
            reload();
        }
    });
}

bool AppUpdateSettings::automaticUpdates() const
{
    return m_automaticUpdates;
}

void AppUpdateSettings::setAutomaticUpdates(bool enabled)
{
    if (m_automaticUpdates == enabled) {
        return;
    }

    m_config->group(QLatin1String(configGroup)).writeEntry(automaticUpdatesKey, enabled);
    m_config->sync();
    m_automaticUpdates = enabled;
    Q_EMIT automaticUpdatesChanged();
}

int AppUpdateSettings::updateInterval() const
{
    return m_updateInterval;
}

void AppUpdateSettings::setUpdateInterval(int seconds)
{
    if (m_updateInterval == seconds) {
        return;
    }

    m_config->group(QLatin1String(configGroup)).writeEntry(updateIntervalKey, seconds);
    m_config->sync();
    m_updateInterval = seconds;
    Q_EMIT updateIntervalChanged();
}

void AppUpdateSettings::reload()
{
    const KConfigGroup group = m_config->group(QLatin1String(configGroup));
    const bool automaticUpdates = group.readEntry(automaticUpdatesKey, true);
    const int updateInterval = group.readEntry(updateIntervalKey, 7 * 24 * 60 * 60);

    if (m_automaticUpdates != automaticUpdates) {
        m_automaticUpdates = automaticUpdates;
        Q_EMIT automaticUpdatesChanged();
    }
    if (m_updateInterval != updateInterval) {
        m_updateInterval = updateInterval;
        Q_EMIT updateIntervalChanged();
    }
}
