/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef IOSSIMULATOR_H
#define IOSSIMULATOR_H

#include <projectexplorer/devicesupport/idevice.h>
#include <utils/fileutils.h>

#include <QMutex>
#include <QDebug>
#include <QSharedPointer>

namespace ProjectExplorer { class Kit; }
namespace Ios {
namespace Internal {
class IosConfigurations;
class IosSimulatorFactory;

class IosDeviceType {
public:
    enum Type {
        IosDevice,
        SimulatedDevice
    };
    IosDeviceType(Type type = IosDevice, const QString &identifier = QString(),
                  const QString &displayName = QString());

    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

    bool operator ==(const IosDeviceType &o) const;
    bool operator !=(const IosDeviceType &o) const { return !(*this == o); }
    bool operator <(const IosDeviceType &o) const;

    Type type;
    QString identifier;
    QString displayName;
};
QDebug operator <<(QDebug debug, const IosDeviceType &deviceType);

class IosSimulator : public ProjectExplorer::IDevice
{
public:
    typedef QSharedPointer<const IosSimulator> ConstPtr;
    typedef QSharedPointer<IosSimulator> Ptr;
    ProjectExplorer::IDevice::DeviceInfo deviceInformation() const Q_DECL_OVERRIDE;

    static QList<IosDeviceType> availableDevices();
    static void setAvailableDevices(QList<IosDeviceType> value);
    static void updateAvailableDevices();

    QString displayType() const Q_DECL_OVERRIDE;
    ProjectExplorer::IDeviceWidget *createWidget() Q_DECL_OVERRIDE;
    QList<Core::Id> actionIds() const Q_DECL_OVERRIDE;
    QString displayNameForActionId(Core::Id actionId) const Q_DECL_OVERRIDE;
    void executeAction(Core::Id actionId, QWidget *parent = 0) Q_DECL_OVERRIDE;
    ProjectExplorer::DeviceProcessSignalOperation::Ptr signalOperation() const Q_DECL_OVERRIDE;
    void fromMap(const QVariantMap &map) Q_DECL_OVERRIDE;
    QVariantMap toMap() const Q_DECL_OVERRIDE;
    quint16 nextPort() const;
    bool canAutoDetectPorts() const Q_DECL_OVERRIDE;

    ProjectExplorer::IDevice::Ptr clone() const Q_DECL_OVERRIDE;
protected:
    friend class IosSimulatorFactory;
    friend class IosConfigurations;
    IosSimulator();
    IosSimulator(Core::Id id);
    IosSimulator(const IosSimulator &other);
private:
    mutable quint16 m_lastPort;
    static QMutex _mutex;
    static QList<IosDeviceType> _availableDevices;
};

namespace IosKitInformation {
IosSimulator::ConstPtr simulator(ProjectExplorer::Kit *kit);
} // namespace IosKitInformation
} // namespace Internal
} // namespace Ios

Q_DECLARE_METATYPE(Ios::Internal::IosDeviceType)

#endif // IOSSIMULATOR_H
