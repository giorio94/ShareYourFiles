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

#ifndef DUPLICATEDFILEMODEL_HPP
#define DUPLICATEDFILEMODEL_HPP

#include <QDateTime>
#include <QObject>
#include <QPointer>

class PeersList;
class SyfftProtocolDuplicatedFile;

///
/// \brief The DuplicatedFileModel class provides the c++ model used by the
/// DuplicatedFile QML window.
///
/// In particular, the class exposes all the main information about the
/// duplicated file (both the on-disk and the received one) and regarding the
/// sender user through ad-hoc properties that can be accessed from QML code.
/// Three slots are also provided to perform the chosen action about the
/// request.
///
class DuplicatedFileModel : public QObject
{
    Q_OBJECT
public:
    /// \brief Provides access to the sender's names.
    Q_PROPERTY(QString names READ names NOTIFY senderNamesUpdated)

    /// \brief Provides access to the path where the file is stored.
    Q_PROPERTY(QString filepath READ filepath CONSTANT)
    /// \brief Provides access to the path where the file is stored.
    Q_PROPERTY(QString filename READ filename CONSTANT)

    /// \brief Provides access to the size of the on-disk file.
    Q_PROPERTY(QString oldFileSize READ oldFileSize CONSTANT)
    /// \brief Provides access to the size of the file to be received.
    Q_PROPERTY(QString newFileSize READ newFileSize CONSTANT)

    /// \brief Provides access to the last modified date of the on-disk file.
    Q_PROPERTY(QString oldFileDate READ oldFileDate CONSTANT)
    /// \brief Provides access to the last modified date of the file to be
    /// received.
    Q_PROPERTY(QString newFileDate READ newFileDate CONSTANT)

    ///
    /// \brief Builds a new instance of this model.
    /// \param request the object representing the duplicated file request.
    /// \param peersList the object representing the list of peers.
    /// \param parent the parent of the current object.
    ///
    explicit DuplicatedFileModel(
        QSharedPointer<SyfftProtocolDuplicatedFile> request,
        PeersList *peersList, QObject *parent = Q_NULLPTR);


    /// \brief Returns the sender's names.
    QString names() const { return m_names; }

    /// \brief Returns the path where the file is stored.
    QString filepath() const;
    /// \brief Returns the path where the file is stored.
    QString filename() const;

    /// \brief Returns the size of the on-disk file.
    QString oldFileSize() const;
    /// \brief Returns the size of the file to be received.
    QString newFileSize() const;

    /// \brief Returns the last modified date of the on-disk file.
    QString oldFileDate() const;
    /// \brief Returns the last modified date of the file to be received.
    QString newFileDate() const;

signals:
    /// \brief Signal emitted when the sender names changes.
    void senderNamesUpdated();

    /// \brief Signal emitted when the instance is requested to be destroyed.
    void requestedDestruction();

    /// \brief Signal emitted if the connection is aborted during the choice.
    void connectionAborted();

public slots:
    ///
    /// \brief Keeps the existing file and discard the received one.
    /// \param all if true, apply also to the other conflicts in the transfer.
    ///
    void keepExisting(bool all);

    ///
    /// \brief Replaces the existing file with the received one.
    /// \param all if true, apply also to the other conflicts in the transfer.
    ///
    void replaceExisting(bool all);

    ///
    /// \brief Keeps both files by adding a suffix to the received one.
    /// \param all if true, apply also to the other conflicts in the transfer.
    ///
    void keepBoth(bool all);

    ///
    /// \brief Requests the destruction of the current object.
    ///
    void requestDestruction() { emit requestedDestruction(); }

private:
    /// \brief Updates the user information about the sender.
    void updateSenderInformation();

private:
    /// \brief The instance storing data about the duplicated file request.
    QSharedPointer<SyfftProtocolDuplicatedFile> m_request;

    /// \brief The instance storing data about the peers.
    QPointer<PeersList> m_peersList;

    QString m_names; ///< \brief Sender's names.
};

#endif // DUPLICATEDFILEMODEL_HPP
