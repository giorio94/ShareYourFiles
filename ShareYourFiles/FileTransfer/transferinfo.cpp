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

#include "transferinfo.hpp"
#include "Common/common.hpp"

///
/// The instance is initialized with default values.
///
TransferInfo::TransferInfo()
        : m_totalFiles(0),
          m_transferredFiles(0),
          m_skippedFiles(0),
          m_totalBytes(0),
          m_transferredBytes(0),
          m_skippedBytes(0),
          m_elapsedTime(0),
          m_transferTime(0),
          m_pausedTime(0),
          m_currentSpeed(qInf()),
          m_fileInTransfer(QString())
{
}

///
/// The percentage is computed considering both transferred and skipped files.
/// In case no files are scheduled for transfer, it is returned 100.
///
float TransferInfo::percentageFiles() const
{
    return (m_totalFiles > 0)
               ? 100 * (m_transferredFiles + m_skippedFiles) / m_totalFiles
               : 100;
}

///
/// The percentage is computed considering both transferred and skipped files.
/// In case the total size of the files is equal to zero, it is returned the
/// percentage of transferred files.
///
float TransferInfo::percentageBytes() const
{
    return (m_totalBytes > 0)
               ? 100 * (m_transferredBytes + m_skippedBytes) / m_totalBytes
               : percentageFiles();
}

///
/// The remaining time is estimated according to the bytes still to be
/// transferred
/// and the average transfer speed until now. If the average speed cannot be
/// determines, the special value Constants::UNKNOWN_INTERVAL is returned.
///
quint64 TransferInfo::remainingTime() const
{
    double averageSpeed = averageTransferSpeed();
    if (qFuzzyIsNull(averageSpeed) || qIsInf(averageSpeed))
        return Constants::UNKNOWN_INTERVAL;

    return static_cast<quint64>(1000 * remainingBytes() / averageSpeed);
}

///
/// The transfer speed is computed as the ratio between the number of bytes
/// transferred until now and the time elapsed. In case no time has elapsed
/// yet, an infinite value is returned.
///
double TransferInfo::transferSpeed(quint64 bytes, qint64 milliseconds) const
{
    if (milliseconds == 0)
        return qInf();

    return static_cast<double>(bytes * 1000) / milliseconds;
}

///
/// The current transfer speed is recomputed and memorized. The computation
/// is performed considering the number of transferred bytes and the elapsed
/// time since the last time this method has been executed.
///
void TransferInfo::recomputeCurrentSpeed(bool reset)
{
    // Recompute the current speed
    m_currentSpeed =
        (reset) ? qInf() : transferSpeed(m_transferredBytes - previousBytes,
                                         m_transferTime - previousTime);


    // Update the previous values
    previousBytes = m_transferredBytes;
    previousTime = m_transferTime;
}
