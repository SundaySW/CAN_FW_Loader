#pragma once

#include "QThread"
#include "QMutex"
#include "QWaitCondition"
#include "protos_message.h"

#include "moodycamel/concurrentqueue.h"

class SocketThread: public QThread{
    Q_OBJECT
    using MsgQueue = moodycamel::ConcurrentQueue<QByteArray>;
public:
    SocketThread(QString ip, qint16 port, QObject *parent = nullptr);
    void run() override;
    void PlaceMsg(ProtosMessage&);
    ~SocketThread();
signals:
    void error(QString error);
    void statusUpdate(QString msg);
    void newData(ProtosMessage& msg);
private:
    const int kServiceBytesCnt = 4,
            kMaxProtosMsgLength = 15,
            kMinPacketLength = 9;

    MsgQueue tx_msg_queue_;
    QByteArray data_buffer_{};

    QString ip_;
    qint16 port_;
    QMutex mutex_;
    QWaitCondition cv_;
    std::atomic<bool> quit_{true};
    void MsgPullOut(QByteArray&);
    static std::pair<QString, ProtosMessage> ConvertDataToMsg(const QByteArray &data);
    QByteArray PackSocketMsg(ProtosMessage &msg);
};