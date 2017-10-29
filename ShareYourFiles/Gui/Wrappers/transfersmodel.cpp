/*
 *  Copyright (c) 2017 Marco Iorio (giorio94 at gmail dot com)
 *  This file is part of Share Your Files (SYF).
 *
 *  SYF is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  SYF is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with SYF.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "transfersmodel.hpp"
#include "FileTransfer/syfftprotocolreceiver.hpp"
#include "FileTransfer/syfftprotocolsender.hpp"
#include "FileTransfer/transferinfo.hpp"
#include "UserDiscovery/user.hpp"
#include "UserDiscovery/users.hpp"

#include "Gui/Wrappers/duplicatedfilemodel.hpp"
#include "Gui/Wrappers/transferrequestmodel.hpp"
#include "Gui/Wrappers/transferresponsemodel.hpp"

#include <QTimer>

// Register TransferRequestModel to the QML system
static QmlTypeRegistration<TransferRequestModel> requestRegisterer;
// Register TransferResponseModel to the QML system
static QmlTypeRegistration<TransferResponseModel> responseRegisterer;
// Register DuplicatedFileModel to the QML system
static QmlTypeRegistration<DuplicatedFileModel> duplicatedFileRegisterer;

// Static variables initialization
static QMap<SyfftProtocolCommon::Status, QString> statusInitializer()
{
    using Status = SyfftProtocolCommon::Status;
    QMap<Status, QString> status;
    status.insert(Status::New, QObject::tr("Ask"));
    status.insert(Status::Connecting, QObject::tr("Connecting to the peer..."));
    status.insert(Status::Connected,
                  QObject::tr("Connection correctly established"));
    status.insert(Status::InTransfer, QObject::tr("Transfer in progress"));
    status.insert(Status::TransferCompleted,
                  QObject::tr("File transfer completed"));
    status.insert(Status::Closing, QObject::tr("Closing the connection..."));
    status.insert(Status::Closed, QObject::tr("Connection correctly closed"));
    status.insert(Status::Aborted, QObject::tr("Connection aborted"));
    status.insert(Status::PausedByUser, QObject::tr("Connection paused"));
    status.insert(Status::PausedByPeer,
                  QObject::tr("Connection paused by the peer"));
    return status;
}
const QMap<SyfftProtocolCommon::Status, QString>
    TransfersModel::STATUS(statusInitializer());

TransfersModel::TransfersModel(LocalUser *localUser, PeersList *peersList,
                               QObject *parent)
        : QAbstractListModel(parent),
          m_localUser(localUser),
          m_peersList(peersList),
          m_timerUpdate(new QTimer(this))
{
    // Initialize the timer used to update the transfer information
    connect(m_timerUpdate, &QTimer::timeout, this, [this]() {
        // Update all the transfer information
        for (int i = 0; i < m_instances.count(); i++) {
            m_transferInformations[i] = m_instances.at(i)->transferInfo();
        }

        // Emit the data changed signal to force the update of the transfer
        // information for each element
        emit dataChanged(createIndex(0, 0),
                         createIndex(m_instances.count(), 0));
    });

    m_timerUpdate->start(UPDATE_INTERVAL);
}

int TransfersModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_instances.count();
}

QVariant TransfersModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_instances.size())
        return QVariant();

    // Transfer information
    const TransferInfo tinfo = m_transferInformations.at(index.row());
    switch (role) {
    case Roles::SenderRole:
        return m_instances.at(index.row())->inherits("SyfftProtocolSender");

    case Roles::StatusRole:
        return TransfersModel::STATUS[m_instances.at(index.row())->status()];
    case Roles::InTransferRole:
        return m_instances.at(index.row())->status() ==
               SyfftProtocolCommon::Status::InTransfer;
    case Roles::ClosedRole:
        return m_instances.at(index.row())->status() ==
                   SyfftProtocolCommon::Status::Closed ||
               m_instances.at(index.row())->status() ==
                   SyfftProtocolCommon::Status::Aborted;
    case Roles::PausedRole:
        return m_instances.at(index.row())->status() ==
               SyfftProtocolCommon::Status::PausedByUser;

    case Roles::PercentageRole:
        return tinfo.percentageBytes();
    case Roles::FilenameRole:
        return tinfo.fileInTransfer();
    case Roles::SpeedRole:
        return speedToHRFormat(tinfo.currentTransferSpeed());
    case Roles::AvgSpeedRole:
        return speedToHRFormat(tinfo.averageTransferSpeed());

    case Roles::TotalNumberRole:
        return tinfo.totalFiles();
    case Roles::TotalSizeRole:
        return sizeToHRFormat(tinfo.totalBytes());
    case Roles::RemainingTimeRole:
        return intervalToHRFormat(tinfo.remainingTime());
    case Roles::RemainingNumberRole:
        return tinfo.remainingFiles();
    case Roles::RemainingSizeRole:
        return sizeToHRFormat(tinfo.remainingBytes());
    case Roles::SkippedNumberRole:
        return tinfo.skippedFiles();
    case Roles::SkippedSizeRole:
        return sizeToHRFormat(tinfo.skippedBytes());
    }

    // Sender information
    QString peerUuid = m_instances.at(index.row())->peerUuid();
    const UserInfo &info = m_peersList->peer(peerUuid);

    // In case the UUID of the peer has not yet been received, return
    // default values
    if (peerUuid == SyfftProtocolCommon::UNKNOWN_UUID || !info.valid()) {
        switch (role) {
        case Roles::NamesRole:
            return User::NO_NAME;
        case Roles::IconSetRole:
            return false;
        case Roles::IconPathRole:
            return QString();
        }

        return QVariant();
    }

    // Otherwise return the values related to the peer
    switch (role) {
    case Roles::NamesRole:
        return info.names();
    case Roles::IconSetRole:
        return info.icon().set();
    case Roles::IconPathRole:
        return info.icon().path();
    }

    return QVariant();
}

void TransfersModel::addSyfftInstance(
    QSharedPointer<SyfftProtocolSender> instance)
{
    addSyfftInstance(qSharedPointerCast<SyfftProtocolCommon>(instance));

    // Connect the handlers to be executed when a request is accepted or
    // rejected.
    const QString &uuid = instance->peerUuid();
    connect(instance.data(), &SyfftProtocolSender::accepted, this,
            [this, uuid](const QString &message) {
                responseReceived(uuid, true, message);
            });
    connect(instance.data(), &SyfftProtocolSender::rejected, this,
            [this, uuid](const QString &message) {
                responseReceived(uuid, false, message);
            });
}

void TransfersModel::addSyfftInstance(
    QSharedPointer<SyfftProtocolReceiver> instance)
{
    addSyfftInstance(qSharedPointerCast<SyfftProtocolCommon>(instance));

    // Accept the connection and set the slots to manage the requests
    instance->acceptConnection(this, "transferRequested", this,
                               "duplicatedFile");
}

int TransfersModel::ongoingTransfers()
{
    int count = 0;
    foreach (const QSharedPointer<SyfftProtocolCommon> instance, m_instances) {
        if (instance->status() != SyfftProtocolCommon::Status::Closed &&
            instance->status() != SyfftProtocolCommon::Status::Aborted) {
            count++;
        }
    }
    return count;
}

int TransfersModel::ongoingReceptions()
{
    int count = 0;
    foreach (const QSharedPointer<SyfftProtocolCommon> instance, m_instances) {
        if (instance->inherits("SyfftProtocolReceiver") &&
            instance->status() != SyfftProtocolCommon::Status::Closed &&
            instance->status() != SyfftProtocolCommon::Status::Aborted) {
            count++;
        }
    }
    return count;
}

void TransfersModel::pauseConnection(const int index, bool setPause)
{
    if (index < 0 || index >= m_instances.size())
        return;

    m_instances.at(index)->changePauseMode(setPause);
}

void TransfersModel::abortConnection(const int index)
{
    if (index < 0 || index >= m_instances.size())
        return;

    m_instances.at(index)->terminateConnection();
}

void TransfersModel::deleteConnection(const int index)
{
    if (index < 0 || index >= m_instances.size())
        return;

    beginRemoveRows(QModelIndex(), index, index);
    m_instances.removeAt(index);
    m_transferInformations.removeAt(index);
    endRemoveRows();

    emit rowCountChanged();
}

void TransfersModel::transferRequested(
    QSharedPointer<SyfftProtocolSharingRequest> request)
{
    // Create the model to handle the request
    QScopedPointer<TransferRequestModel> model(
        new TransferRequestModel(request, m_localUser, m_peersList));

    // If necessary, emit the signal to request the user how to proceed
    if (model->requestUser()) {
        // Connect the slot to delete the instance when requested
        connect(model.data(), &TransferRequestModel::requestedDestruction,
                model.data(), &QObject::deleteLater);
        // Emit the signal
        emit transferRequestedAsk(model.take());
    }
}

void TransfersModel::responseReceived(const QString &uuid, bool accepted,
                                      const QString &message)
{
    // Create the model to handle the request
    QPointer<TransferResponseModel> model =
        new TransferResponseModel(uuid, m_peersList, accepted, message);

    // Connect the slot to delete the instance when requested
    connect(model, &TransferResponseModel::requestedDestruction, model,
            &QObject::deleteLater);

    // Emit the signal
    emit transferResponseReceived(model);
}

void TransfersModel::duplicatedFile(
    QSharedPointer<SyfftProtocolDuplicatedFile> request)
{
    // Create the model to handle the request
    QPointer<DuplicatedFileModel> model =
        new DuplicatedFileModel(request, m_peersList);

    // Connect the slot to delete the instance when requested
    connect(model, &DuplicatedFileModel::requestedDestruction, model,
            &QObject::deleteLater);

    // Emit the signal
    emit duplicatedFileDetected(model);
}

void TransfersModel::addSyfftInstance(
    QSharedPointer<SyfftProtocolCommon> instance)
{
    int index = rowCount();
    beginInsertRows(QModelIndex(), index, index);
    m_instances << instance;
    m_transferInformations << instance->transferInfo();
    endInsertRows();

    emit rowCountChanged();
}

QHash<int, QByteArray> TransfersModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Roles::NamesRole] = "names";
    roles[Roles::IconSetRole] = "iconSet";
    roles[Roles::IconPathRole] = "iconPath";

    roles[Roles::SenderRole] = "sender";
    roles[Roles::StatusRole] = "status";
    roles[Roles::InTransferRole] = "inTransfer";
    roles[Roles::ClosedRole] = "closed";
    roles[Roles::PausedRole] = "paused";
    roles[Roles::PercentageRole] = "percentage";
    roles[Roles::FilenameRole] = "filename";
    roles[Roles::SpeedRole] = "speed";
    roles[Roles::AvgSpeedRole] = "avgSpeed";

    roles[Roles::TotalNumberRole] = "totalNumber";
    roles[Roles::TotalSizeRole] = "totalSize";
    roles[Roles::RemainingTimeRole] = "remainingTime";
    roles[Roles::RemainingNumberRole] = "remainingNumber";
    roles[Roles::RemainingSizeRole] = "remainingSize";
    roles[Roles::SkippedNumberRole] = "skippedNumber";
    roles[Roles::SkippedSizeRole] = "skippedSize";

    return roles;
}
