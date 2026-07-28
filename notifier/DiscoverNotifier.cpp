/*
 *   SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "DiscoverNotifier.h"
#include "BackendNotifierFactory.h"
#include "UnattendedUpdates.h"
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KPluginFactory>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDebug>
#include <QNetworkInformation>
#include <QProcess>

#include <KIO/ApplicationLauncherJob>
#include <KIO/CommandLauncherJob>

#include "../libdiscover/utils.h"
#include "../libdiscover/UpdateState.h"
#include "../libdiscover/UpdateModel/RefreshNotifierDBus.h"
#include "Login1ManagerInterface.h"
#include "updatessettings.h"
#include <chrono>
#include <algorithm>

#include "debug.h"

using namespace std::chrono_literals;
using namespace Qt::Literals;

DiscoverNotifier::DiscoverNotifier(const std::chrono::seconds &checkDelay, QObject *parent)
    : QObject(parent)
    , m_stateConfig(u"discovernotifierstaterc"_s, KConfig::SimpleConfig, QStandardPaths::GenericStateLocation)
{
    m_settings = std::make_unique<UpdatesSettings>();
    m_settingsWatcher = KConfigWatcher::create(m_settings->sharedConfig());

    KConfigGroup stateGroup = m_stateConfig.group(u"Global"_s);
    settings()->config()->group(u"Global"_s).moveValuesTo({"LastNotificationTime"}, stateGroup);
    settings()->save();

    QNetworkInformation::loadBackendByFeatures(QNetworkInformation::Feature::Reachability | QNetworkInformation::Feature::TransportMedium);
    if (auto info = QNetworkInformation::instance()) {
        connect(info, &QNetworkInformation::reachabilityChanged, this, [this, checkDelay] {
            Q_EMIT stateChanged();
            if (automaticUpdatesEnabled()) {
                const UpdateState::State state = UpdateState::read();
                if (state.pending) {
                    UpdateState::postponeUntil(QDateTime::currentDateTimeUtc().addSecs(30));
                }
                QTimer::singleShot(std::max(checkDelay, 30s), this, &DiscoverNotifier::evaluateAutomaticUpdates);
            }
        });
    } else {
        qWarning() << "QNetworkInformation has no backend. Is NetworkManager.service running?";
    }

    connect(m_settingsWatcher.data(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &group, const QByteArrayList &names) {
        if (group.config()->name() != m_settings->config()->name() || group.name() != QLatin1String("Global")) {
            return;
        }
        if (names.contains("UseUnattendedUpdates") || names.contains("RequiredNotificationInterval")) {
            evaluateAutomaticUpdates();
            Q_EMIT stateChanged();
        }
    });

    m_backends = BackendNotifierFactory().allBackends();
    for (BackendNotifierModule *module : std::as_const(m_backends)) {
        connect(module, &BackendNotifierModule::foundUpdates, this, &DiscoverNotifier::updateStatusNotifier);
        connect(module, &BackendNotifierModule::foundUpgradeAction, this, &DiscoverNotifier::foundUpgradeAction);
        connect(module, &BackendNotifierModule::checkCompleted, this, &DiscoverNotifier::handleCheckCompleted);
    }
    connect(&m_timer, &QTimer::timeout, this, &DiscoverNotifier::showUpdatesNotification);
    m_timer.setSingleShot(true);
    m_timer.setInterval(1s);
    m_scheduleTimer.setSingleShot(true);
    connect(&m_scheduleTimer, &QTimer::timeout, this, &DiscoverNotifier::evaluateAutomaticUpdates);
    updateStatusNotifier();

    if (automaticUpdatesEnabled()) {
        QTimer::singleShot(checkDelay, this, &DiscoverNotifier::evaluateAutomaticUpdates);
    }

    auto login1 = new OrgFreedesktopLogin1ManagerInterface(QStringLiteral("org.freedesktop.login1"),
                                                           QStringLiteral("/org/freedesktop/login1"),
                                                           QDBusConnection::systemBus(),
                                                           this);
    connect(login1, &OrgFreedesktopLogin1ManagerInterface::PrepareForSleep, this, [this, checkDelay](bool sleeping) {
        if (!sleeping && automaticUpdatesEnabled() && automaticUpdateDue()) {
            QTimer::singleShot(checkDelay, this, &DiscoverNotifier::evaluateAutomaticUpdates);
        }
    });

    // Listen to broadcasts from discover about notification changes.
    QDBusConnection::sessionBus().connect(QString(),
                                          RefreshNotifierDBus::path,
                                          RefreshNotifierDBus::interface,
                                          RefreshNotifierDBus::notifyNotifier,
                                          this,
                                          SLOT(recheckSystemUpdateNeeded()));
}

DiscoverNotifier::~DiscoverNotifier() = default;

void DiscoverNotifier::showDiscover(const QString &xdgActivationToken)
{
    auto *job = new KIO::ApplicationLauncherJob(KService::serviceByDesktopName(QStringLiteral("org.kde.discover")));
    job->setStartupId(xdgActivationToken.toUtf8());
    job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoErrorHandlingEnabled));
    job->start();

    if (m_updatesAvailableNotification) {
        m_updatesAvailableNotification->close();
    }
}

void DiscoverNotifier::showDiscoverUpdates(const QString &xdgActivationToken)
{
    auto *job = new KIO::CommandLauncherJob(QStringLiteral("plasma-discover"), {QStringLiteral("--mode"), QStringLiteral("update")});
    job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoErrorHandlingEnabled));
    job->setDesktopName(QStringLiteral("org.kde.discover"));
    job->setStartupId(xdgActivationToken.toUtf8());
    job->start();

    if (m_updatesAvailableNotification) {
        m_updatesAvailableNotification->close();
    }
}

bool DiscoverNotifier::checkTriggerTimes(const QDateTime &lastTriggerTime) const
{
    if (state() != NormalUpdates) {
        // it's not very helpful to notify that everything is in order
        qCDebug(NOTIFIER) << "Not triggering, state is" << state();
        return false;
    }

    if (m_settings->requiredNotificationInterval() < 0) {
        qCDebug(NOTIFIER) << "Not triggering, requiredNotificationInterval is" << m_settings->requiredNotificationInterval();
        return false;
    }

    // To configure to a random value, execute:
    // kwriteconfig5 --file PlasmaDiscoverUpdates --group Global --key RequiredNotificationInterval 3600
    const QDateTime earliestNextTriggerTime = lastTriggerTime.addSecs(m_settings->requiredNotificationInterval());
    if (earliestNextTriggerTime.isValid() && earliestNextTriggerTime > QDateTime::currentDateTimeUtc()) {
        qCDebug(NOTIFIER) << "Not triggering, earliestNextTriggerTime is" << earliestNextTriggerTime;
        return false;
    }

    return true;
}

QDateTime DiscoverNotifier::lastNotificationTime() const
{
    return m_stateConfig.group(u"Global"_s).readEntry("LastNotificationTime", QDateTime());
}

bool DiscoverNotifier::notifyAboutUpdates()
{
    if (!automaticUpdatesEnabled()) {
        return false;
    }
    if (!checkTriggerTimes(lastNotificationTime())) {
        return false;
    }

    m_stateConfig.group(u"Global"_s).writeEntry("LastNotificationTime", QDateTime::currentDateTimeUtc());
    m_stateConfig.sync();

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.kde.discover"))) {
        qCDebug(NOTIFIER) << "Not notifying about updates, discover is running";
        return false;
    }
    return true;
}

bool DiscoverNotifier::proceedUnattended() const
{
    if (!automaticUpdatesEnabled() || !automaticUpdateDue() || !m_hasReachableSources || !m_hasConfiguredSources) {
        return false;
    }

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.kde.discover"))) {
        qCDebug(NOTIFIER) << "Not proceeding with unattended update, discover is running";
        return false;
    }
    return true;
}

void DiscoverNotifier::showUpdatesNotification()
{
    if (m_updatesAvailableNotification) {
        m_updatesAvailableNotification->close();
    }

    if (!notifyAboutUpdates()) {
        qCDebug(NOTIFIER) << "showUpdatesNotification: not notifying about updates";
        return;
    }
    qCDebug(NOTIFIER) << "showUpdatesNotification: notifying about updates";

    m_updatesAvailableNotification =
        KNotification::event(QStringLiteral("Update"), message(), {}, iconName(), KNotification::CloseOnTimeout, QStringLiteral("discoverabstractnotifier"));
    m_updatesAvailableNotification->setHint(QStringLiteral("resident"), true);
    const QString name = i18n("View Updates");

    auto showUpdates = [this] {
        showDiscoverUpdates(m_updatesAvailableNotification->xdgActivationToken());
    };

    auto defaultAction = m_updatesAvailableNotification->addDefaultAction(name);
    connect(defaultAction, &KNotificationAction::activated, this, showUpdates);

    auto showUpdatesAction = m_updatesAvailableNotification->addAction(name);
    connect(showUpdatesAction, &KNotificationAction::activated, this, showUpdates);
}

void DiscoverNotifier::updateStatusNotifier()
{
    const bool hasUpdates = kContains(m_backends, [](BackendNotifierModule *module) {
        return module->hasUpdates();
    });

    qCDebug(NOTIFIER) << "updateStatusNotifier: hasUpdates" << hasUpdates;

    if (m_hasUpdates == hasUpdates)
        return;

    m_hasUpdates = hasUpdates;

    if (state() != NoUpdates) {
        m_timer.start();
    }

    Q_EMIT stateChanged();
}

bool DiscoverNotifier::isSystemUpdateable() const
{
    const bool updateable = !m_isBusy && m_hasConfiguredSources && m_hasReachableSources && m_hasUpdates && automaticUpdateDue();
    qCDebug(NOTIFIER) << "isSystemUpdateable:" << updateable << "isBusy:" << m_isBusy << "sources:" << m_hasConfiguredSources
                      << "reachable:" << m_hasReachableSources << "updates:" << m_hasUpdates;
    return updateable;
}

void DiscoverNotifier::startUnattendedUpdates()
{
    auto process = new QProcess(this);
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        if (process->property("flufflinuxHandled").toBool()) {
            return;
        }
        process->setProperty("flufflinuxHandled", true);
        qWarning() << "Error running plasma-discover" << error;
        UpdateState::recordFailure(i18n("An error occurred while updating apps."), 15 * 60);
        process->deleteLater();
        m_unattended.reset();
        setBusy(false);
        scheduleNextEvaluation();
    });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
        if (process->property("flufflinuxHandled").toBool()) {
            return;
        }
        process->setProperty("flufflinuxHandled", true);
        qDebug() << "Finished running plasma-discover" << exitCode << exitStatus;
        process->deleteLater();
        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            const int retryDelays[] = {15 * 60, 60 * 60, 6 * 60 * 60};
            const int retryIndex = std::min(UpdateState::read().retryCount, 2);
            UpdateState::recordFailure(i18n("An error occurred while updating apps."), retryDelays[retryIndex]);
        }
        m_unattended.reset();
        setBusy(false);
        scheduleNextEvaluation();
    });

    setBusy(true);
    UpdateState::recordAttempt();
    process->start(QStringLiteral("plasma-discover"), {QStringLiteral("--headless-update")});
    qInfo() << "started unattended update" << QDateTime::currentDateTimeUtc();
}

void DiscoverNotifier::refreshUnattended()
{
    m_settings->read();

    if (!automaticUpdatesEnabled()) {
        m_unattended.reset();
        m_scheduleTimer.stop();
        m_timer.stop();
        if (m_updatesAvailableNotification) {
            m_updatesAvailableNotification->close();
        }
        return;
    }

    const bool enabled = proceedUnattended() && isSystemUpdateable();
    if (bool(m_unattended) == enabled)
        return;

    if (enabled) {
        qCDebug(NOTIFIER) << "Enabling unattended updates";
        m_unattended = std::make_unique<UnattendedUpdates>(this);
    } else {
        m_unattended.reset();
    }
    scheduleNextEvaluation();
}

bool DiscoverNotifier::automaticUpdatesEnabled() const
{
    return m_settings && m_settings->useUnattendedUpdates() && m_settings->requiredNotificationInterval() > 0;
}

bool DiscoverNotifier::automaticUpdateDue() const
{
    if (!automaticUpdatesEnabled()) {
        return false;
    }
    const UpdateState::State state = UpdateState::read();
    if (state.nextScheduledUpdate.isValid()) {
        return state.nextScheduledUpdate <= QDateTime::currentDateTimeUtc();
    }
    if (!state.lastSuccess.isValid()) {
        return true;
    }
    return state.lastSuccess.addSecs(m_settings->requiredNotificationInterval()) <= QDateTime::currentDateTimeUtc();
}

void DiscoverNotifier::scheduleNextEvaluation()
{
    m_scheduleTimer.stop();
    if (!automaticUpdatesEnabled() || m_isBusy || m_checkInProgress || (m_sourceStateKnown && !m_hasConfiguredSources)) {
        return;
    }

    const UpdateState::State state = UpdateState::read();
    QDateTime next = state.nextScheduledUpdate;
    if (!next.isValid() && state.lastSuccess.isValid()) {
        next = state.lastSuccess.addSecs(m_settings->requiredNotificationInterval());
    }
    if (!next.isValid() || next <= QDateTime::currentDateTimeUtc()) {
        m_scheduleTimer.start(1000);
        return;
    }

    const qint64 milliseconds = QDateTime::currentDateTimeUtc().msecsTo(next);
    m_scheduleTimer.start(int(std::min<qint64>(milliseconds, 24LL * 60 * 60 * 1000)));
}

void DiscoverNotifier::evaluateAutomaticUpdates()
{
    m_settings->read();
    if (!automaticUpdatesEnabled()) {
        refreshUnattended();
        return;
    }
    if (!automaticUpdateDue() || m_checkInProgress || m_isBusy) {
        scheduleNextEvaluation();
        return;
    }
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.kde.discover"))) {
        UpdateState::postponeUntil(QDateTime::currentDateTimeUtc().addSecs(15 * 60));
        scheduleNextEvaluation();
        return;
    }

    m_checkInProgress = true;
    recheckSystemUpdateNeeded();
}

void DiscoverNotifier::handleCheckCompleted(bool hasConfiguredSources, bool hasReachableSources)
{
    m_checkInProgress = false;
    m_sourceStateKnown = true;
    m_hasConfiguredSources = hasConfiguredSources;
    m_hasReachableSources = hasReachableSources;
    updateStatusNotifier();

    if (!automaticUpdatesEnabled()) {
        refreshUnattended();
        return;
    }
    if (!hasConfiguredSources) {
        m_unattended.reset();
        m_scheduleTimer.stop();
        return;
    }
    if (!hasReachableSources) {
        UpdateState::postponeUntil(QDateTime::currentDateTimeUtc().addSecs(15 * 60));
        m_unattended.reset();
        scheduleNextEvaluation();
        return;
    }

    UpdateState::recordCheck();
    if (!m_hasUpdates) {
        UpdateState::recordNoUpdates(m_settings->requiredNotificationInterval());
        m_unattended.reset();
        scheduleNextEvaluation();
        return;
    }
    refreshUnattended();
}

DiscoverNotifier::State DiscoverNotifier::state() const
{
    if (m_isBusy)
        return Busy;
    else if (automaticUpdatesEnabled() && m_hasConfiguredSources && !m_hasReachableSources)
        return Offline;
    else if (m_hasUpdates)
        return NormalUpdates;
    else
        return NoUpdates;
}

QString DiscoverNotifier::iconName() const
{
    return QStringLiteral("flufflinuxplasmadiscover");
}

QString DiscoverNotifier::message() const
{
    switch (state()) {
    case NormalUpdates:
        return i18n("App updates available");
    case NoUpdates:
        return i18n("System up to date");
    case Offline:
        return i18n("Offline");
    case Busy:
        return i18n("Applying unattended updates…");
    }
    return QString();
}

void DiscoverNotifier::recheckSystemUpdateNeeded()
{
    if (!automaticUpdatesEnabled()
        && !QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.kde.discover"))) {
        return;
    }
    m_lastUpdate = QDateTime::currentDateTimeUtc();
    for (BackendNotifierModule *module : std::as_const(m_backends))
        module->recheckSystemUpdateNeeded();
}

QStringList DiscoverNotifier::loadedModules() const
{
    QStringList ret;
    for (BackendNotifierModule *module : m_backends)
        ret += QString::fromLatin1(module->metaObject()->className());
    return ret;
}

void DiscoverNotifier::promptAll()
{
    auto method = QDBusMessage::createMethodCall(QStringLiteral("org.kde.LogoutPrompt"),
                                                 QStringLiteral("/LogoutPrompt"),
                                                 QStringLiteral("org.kde.LogoutPrompt"),
                                                 QStringLiteral("promptAll"));
    QDBusConnection::sessionBus().asyncCall(method);
}

void DiscoverNotifier::foundUpgradeAction(UpgradeAction *action)
{
    updateStatusNotifier();

    if (!notifyAboutUpdates()) {
        return;
    }

    KNotification *notification = new KNotification(QStringLiteral("DistUpgrade"), KNotification::Persistent);
    notification->setIconName(QStringLiteral("flufflinuxplasmadiscover"));
    notification->setTitle(i18n("Upgrade available"));
    notification->setText(i18nc("A new distro release (name and version) is available for upgrade", "%1 is now available.", action->description()));
    notification->setComponentName(QStringLiteral("discoverabstractnotifier"));

    auto upgradeAction = notification->addAction(i18nc("@action:button", "Upgrade"));
    connect(upgradeAction, &KNotificationAction::activated, this, [action] {
        action->trigger();
    });

    connect(action, &UpgradeAction::showDiscoverUpdates, this, [this, notification]() {
        showDiscoverUpdates(notification->xdgActivationToken());
    });

    notification->sendEvent();
}

void DiscoverNotifier::setBusy(bool isBusy)
{
    if (isBusy == m_isBusy)
        return;

    m_isBusy = isBusy;
    Q_EMIT busyChanged();
    Q_EMIT stateChanged();
}

void DiscoverNotifier::recheckSystemUpdateNeededAndNotifyApp()
{
    recheckSystemUpdateNeeded();
    RefreshNotifierDBus::notify(RefreshNotifierDBus::notifyApp);
}

#include "moc_DiscoverNotifier.cpp"
