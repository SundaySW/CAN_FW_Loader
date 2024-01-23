#pragma once

#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/QLabel>
#include "device_holder.hpp"
#include "protos_message.h"

class FWLoader: public QObject
{
    Q_OBJECT
public:
    explicit FWLoader(const QSharedPointer<Tcp_socket>& socket);
    void ParseBootMsg(const ProtosMessage &msg);
    void CancelFWLoad(uint uid);
    void RefreshDevice(uint uid);
    void FinishDevice(uint uid, qint64 msecs);
signals:
    void signalAckReceived(uint uid, uchar addr, uchar hw, uchar fw);
    void signalNextBlock(uint delta, uint uid, uint addr);
    void signalFinishedOK(uint uid, qint64 msecs);
    void signalError(const QString& error, uint uid);
public slots:
    void addDevice(const QString&, uchar, uint, uchar, uchar);
private:
    QMap<uint32_t, DeviceHolder> device_list_;
    QSharedPointer<Tcp_socket> socket_;
};