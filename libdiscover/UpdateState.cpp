/*
 * SPDX-FileCopyrightText: 2026 FluffNet LLC
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "UpdateState.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

namespace
{
QDateTime readDate(const QJsonObject &object, const char *key)
{
    return QDateTime::fromString(object.value(QLatin1String(key)).toString(), Qt::ISODateWithMs);
}

QString writeDate(const QDateTime &date)
{
    return date.isValid() ? date.toUTC().toString(Qt::ISODateWithMs) : QString();
}
}

QString UpdateState::path()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation)
        + QStringLiteral("/flufflinux-discover/update-state.json");
}

UpdateState::State UpdateState::read()
{
    QFile file(path());
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return {};
    }

    const QJsonObject object = document.object();
    return {
        .lastCheck = readDate(object, "last_check"),
        .lastAttempt = readDate(object, "last_attempt"),
        .lastSuccess = readDate(object, "last_successful_update"),
        .lastFailure = readDate(object, "last_failure"),
        .nextScheduledUpdate = readDate(object, "next_scheduled_update"),
        .lastError = object.value(QStringLiteral("last_error")).toString(),
        .retryCount = object.value(QStringLiteral("retry_count")).toInt(),
        .pending = object.value(QStringLiteral("pending")).toBool(),
    };
}

bool UpdateState::write(const State &state)
{
    const QFileInfo info(path());
    if (!QDir().mkpath(info.absolutePath())) {
        return false;
    }

    const QJsonObject object{
        {QStringLiteral("last_check"), writeDate(state.lastCheck)},
        {QStringLiteral("last_attempt"), writeDate(state.lastAttempt)},
        {QStringLiteral("last_successful_update"), writeDate(state.lastSuccess)},
        {QStringLiteral("last_failure"), writeDate(state.lastFailure)},
        {QStringLiteral("next_scheduled_update"), writeDate(state.nextScheduledUpdate)},
        {QStringLiteral("last_error"), state.lastError},
        {QStringLiteral("retry_count"), state.retryCount},
        {QStringLiteral("pending"), state.pending},
    };

    QSaveFile file(path());
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    return file.commit();
}

void UpdateState::recordCheck()
{
    State state = read();
    state.lastCheck = QDateTime::currentDateTimeUtc();
    write(state);
}

void UpdateState::recordAttempt()
{
    State state = read();
    state.lastAttempt = QDateTime::currentDateTimeUtc();
    state.pending = true;
    write(state);
}

void UpdateState::recordSuccess(int intervalSeconds)
{
    State state = read();
    const QDateTime now = QDateTime::currentDateTimeUtc();
    state.lastCheck = now;
    state.lastSuccess = now;
    state.lastError.clear();
    state.lastFailure = {};
    state.retryCount = 0;
    state.pending = false;
    state.nextScheduledUpdate = intervalSeconds > 0 ? now.addSecs(intervalSeconds) : QDateTime();
    write(state);
}

void UpdateState::recordNoUpdates(int intervalSeconds)
{
    State state = read();
    const QDateTime now = QDateTime::currentDateTimeUtc();
    state.lastCheck = now;
    state.lastError.clear();
    state.retryCount = 0;
    state.pending = false;
    state.nextScheduledUpdate = intervalSeconds > 0 ? now.addSecs(intervalSeconds) : QDateTime();
    write(state);
}

void UpdateState::recordFailure(const QString &error, int retryDelaySeconds)
{
    State state = read();
    const QDateTime now = QDateTime::currentDateTimeUtc();
    state.lastFailure = now;
    state.lastError = error;
    state.retryCount++;
    state.pending = true;
    state.nextScheduledUpdate = now.addSecs(retryDelaySeconds);
    write(state);
}

void UpdateState::postponeUntil(const QDateTime &when)
{
    State state = read();
    state.pending = true;
    state.nextScheduledUpdate = when.toUTC();
    write(state);
}
