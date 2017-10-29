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

#ifndef TRANSFERINFO_HPP
#define TRANSFERINFO_HPP

#include <QString>

///
/// \brief The TransferInfo class provides statistics about the transfer
/// progress
///
/// This class stores many information related to how is proceeding the file
/// transfer managed by the SYFFT Protocol: in particular, it is possible to
/// query the total, transferred, skipped and remaining number of files and
/// bytes, the total elapsed time, the time spent in transfer and in pause,
/// and the estimated remaining time, the average and the current speed.
///
/// The class is designed to provide read-only access to the various fields,
/// while the modification possibility is granted only to the classes
/// implementing the SYFFT protocol through a friendship link.
///
class TransferInfo
{
    friend class SyfftProtocolCommon;
    friend class SyfftProtocolReceiver;
    friend class SyfftProtocolSender;

public:
    ///
    /// \brief Generates an new instance.
    ///
    explicit TransferInfo();

    ///
    /// \brief Generates an instance which is an exact copy of the parameter.
    /// \param other the instance to be copied.
    ///
    TransferInfo(const TransferInfo &other) = default;

    ///
    /// \brief Copies the data from the parameter to the current instance.
    /// \param other the instance to be copied.
    ///
    TransferInfo &operator=(const TransferInfo &other) = default;

    ///
    /// \brief Frees the memory used by the instance.
    ///
    ~TransferInfo() = default;

    /// \brief Returns the total number of files.
    quint32 totalFiles() const { return m_totalFiles; }
    /// \brief Returns the number of files already transferred correctly.
    quint32 transferredFiles() const { return m_transferredFiles; }
    /// \brief Returns the number of skipped files (due to refusal or error).
    quint32 skippedFiles() const { return m_skippedFiles; }
    /// \brief Returns the number of files still to be transferred.
    quint32 remainingFiles() const
    {
        return m_totalFiles - m_transferredFiles - m_skippedFiles;
    }
    /// \brief Returns the percentage of files already transferred or skipped.
    float percentageFiles() const;

    /// \brief Returns the total size of the files.
    quint64 totalBytes() const { return m_totalBytes; }
    /// \brief Returns the amount of bytes already transferred correctly.
    quint64 transferredBytes() const { return m_transferredBytes; }
    /// \brief Returns the amount of skipped bytes (due to refusal or error).
    quint64 skippedBytes() const { return m_skippedBytes; }
    /// \brief Returns the amount of bytes still to be transferred.
    quint64 remainingBytes() const
    {
        return m_totalBytes - m_transferredBytes - m_skippedBytes;
    }
    /// \brief Returns the percentage of bytes already transferred or skipped.
    float percentageBytes() const;

    /// \brief Returns the total elapsed time.
    qint64 elapsedTime() const { return m_elapsedTime; }
    /// \brief Returns the elapsed time spent in transfer.
    qint64 transferTime() const { return m_transferTime; }
    /// \brief Returns the elapsed time spent in pause mode.
    qint64 pausedTime() const { return m_pausedTime; }
    /// \brief Returns the estimated remaining transfer time.
    quint64 remainingTime() const;

    /// \brief Returns the average transfer speed (Bytes/s).
    double averageTransferSpeed() const
    {
        return transferSpeed(m_transferredBytes, m_transferTime);
    }
    /// \brief Returns the current transfer speed (Bytes/s).
    double currentTransferSpeed() const { return m_currentSpeed; }

    /// \brief Returns relative path of the file currently in transfer.
    QString fileInTransfer() const { return m_fileInTransfer; }

private:
    ///
    /// \brief Computes a transfer speed.
    /// \param bytes the number of bytes transferred.
    /// \param milliseconds the number of milliseconds elapsed.
    /// \return the transfer speed (as Bytes/s).
    ///
    double transferSpeed(quint64 bytes, qint64 milliseconds) const;

    ///
    /// \brief Recomputes the current transfer speed.
    /// \param reset specifies if the cached values must be reset.
    ///
    void recomputeCurrentSpeed(bool reset = false);

private:
    /// \brief The total number of files.
    quint32 m_totalFiles;
    /// \brief The number of files already transferred correctly.
    quint32 m_transferredFiles;
    /// \brief The number of skipped files (due to refusal or error).
    quint32 m_skippedFiles;

    quint64 m_totalBytes; ///< \brief The total size of the files.
    /// \brief The amount of bytes already transferred correctly.
    quint64 m_transferredBytes;
    quint64 m_skippedBytes; ///< \brief The amount of skipped bytes.

    qint64 m_elapsedTime;  ///< \brief The total elapsed time.
    qint64 m_transferTime; ///< \brief The elapsed time spent in transfer.
    qint64 m_pausedTime;   ///< \brief The elapsed time spent in pause mode.

    /// \brief Cached value used for currentSpeed computation.
    quint64 previousBytes;
    /// \brief Cached value used for currentSpeed computation.
    qint64 previousTime;

    double m_currentSpeed; ///< \brief The current transfer speed (Bytes/s).

    /// \brief The relative path of the file currently in transfer.
    QString m_fileInTransfer;
};

#endif // TRANSFERINFO_HPP
