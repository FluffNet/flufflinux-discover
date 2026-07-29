/*
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AppUpdateSettings.h"

#include <KConfigGroup>
#include <KFormat>
#include <KLocalizedString>
#include <UpdateConfig.h>
#include <UpdateState.h>

#include <algorithm>

using namespace Qt::Literals;

namespace
{
constexpr auto configFile = "PlasmaDiscoverUpdates";
constexpr auto configGroup = "Global";
constexpr auto automaticUpdatesKey = "AutomaticUpdates";
constexpr auto updateIntervalKey = "UpdateInterval";
}

AppUpdateSettings::AppUpdateSettings(QObject *parent)
    : QObject(parent)
    , m_config(KSharedConfig::openConfig(QLatin1String(configFile)))
    , m_watcher(KConfigWatcher::create(m_config))
    , m_sessionStarted(QDateTime::currentDateTimeUtc())
{
    ensureDefaultsExist();

    reload();
    connect(m_watcher.data(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &group, const QByteArrayList &names) {
        if (group.name() == QLatin1String(configGroup)
            && (names.contains(automaticUpdatesKey) || names.contains(updateIntervalKey))) {
            reload();
        }
    });
}

void AppUpdateSettings::ensureDefaultsExist()
{
    UpdateConfig::ensureAndMigrate();
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

    UpdateConfig::writeAutomaticUpdates(enabled, m_updateInterval);
    m_automaticUpdates = enabled;
    Q_EMIT automaticUpdatesChanged();
}

int AppUpdateSettings::updateInterval() const
{
    return m_updateInterval;
}

bool AppUpdateSettings::hasLastSuccessfulUpdate() const
{
    return UpdateState::read().lastSuccess.isValid();
}

QString AppUpdateSettings::lastSuccessfulUpdate() const
{
    const QDateTime lastSuccess = UpdateState::read().lastSuccess;
    return lastSuccess.isValid() ? lastSuccess.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss t tt")) : QString();
}

QString AppUpdateSettings::relativeLastSuccessfulUpdate() const
{
    const QDateTime lastSuccess = UpdateState::read().lastSuccess;
    return lastSuccess.isValid() ? KFormat().formatRelativeDateTime(lastSuccess.toLocalTime(), QLocale::LongFormat) : QString();
}

bool AppUpdateSettings::lastUpdateOlderThanWeek() const
{
    const QDateTime lastSuccess = UpdateState::read().lastSuccess;
    return lastSuccess.isValid() && lastSuccess.addDays(7) < QDateTime::currentDateTimeUtc();
}

bool AppUpdateSettings::checkedThisSession() const
{
    const QDateTime lastCheck = UpdateState::read().lastCheck;
    return m_checkedThisSession || (lastCheck.isValid() && lastCheck >= m_sessionStarted);
}

void AppUpdateSettings::recordUpdateCheck()
{
    m_checkedThisSession = true;
    UpdateState::recordCheck();
    Q_EMIT updateHistoryChanged();
}

void AppUpdateSettings::recordUpdateSuccess()
{
    UpdateState::recordSuccess(automaticUpdates() ? updateInterval() : 0);
    Q_EMIT updateHistoryChanged();
}

void AppUpdateSettings::recordUpdateFailure(const QString &error)
{
    const int retryDelays[] = {15 * 60, 60 * 60, 6 * 60 * 60};
    const int retryIndex = std::min(UpdateState::read().retryCount, 2);
    UpdateState::recordFailure(error, retryDelays[retryIndex]);
    Q_EMIT updateHistoryChanged();
}

void AppUpdateSettings::reloadUpdateHistory()
{
    Q_EMIT updateHistoryChanged();
}

void AppUpdateSettings::setUpdateInterval(int seconds)
{
    if (m_updateInterval == seconds) {
        return;
    }

    UpdateConfig::writeUpdateInterval(seconds);
    m_updateInterval = seconds;
    Q_EMIT updateIntervalChanged();
}

void AppUpdateSettings::reload()
{
    const UpdateConfig::Settings settings = UpdateConfig::read();
    const bool automaticUpdates = settings.automaticUpdates;
    const int updateInterval = settings.intervalSeconds;

    if (m_automaticUpdates != automaticUpdates) {
        m_automaticUpdates = automaticUpdates;
        Q_EMIT automaticUpdatesChanged();
    }
    if (m_updateInterval != updateInterval) {
        m_updateInterval = updateInterval;
        Q_EMIT updateIntervalChanged();
    }
}
