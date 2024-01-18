#pragma once

#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/QLabel>
#include "device_holder.hpp"
#include "protos_message.h"

class FWLoader: public QObject
{
    Q_OBJECT
public:
    FWLoader(const QSharedPointer<Tcp_socket>& socket);
    void transmitBlocks();
    void transmitBlock(uint UID);
    void ParseBootMsg(const ProtosMessage &msg);
    void cancelFWLoad(uint UID);
signals:
    void signalAckReceived();
    void signalNextBlock(uint delta, uint uid, uint addr);
    void signalFinishedOK(uint uid, int msecs);
    void signalBootData(uint UID);
    void signalError(const QString& error, uint uid);
public slots:
    void addDevice(const QString&, uchar, uint, uchar, uchar);
private:
    void removeDevice(uint uid, int msecs);
    QMap<uint32_t, DeviceHolder> deviceList;
    QSharedPointer<Tcp_socket> socket_;
    volatile bool busy;
};