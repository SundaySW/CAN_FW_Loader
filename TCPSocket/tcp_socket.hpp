#pragma once

#include <QTcpSocket>
#include <QThread>
#include <QMutex>

#include "moodycamel/concurrentqueue.h"

#include "protos_message.h"

#include "SocketThread.hpp"

class Tcp_socket : public QObject{
Q_OBJECT
    using MsgQueue = moodycamel::ConcurrentQueue<ProtosMessage>;
    using MsgHandlerT = std::function<void(const ProtosMessage&)>;
    using ErrorHandlerT = std::function<void(const QString&)>;
signals:
    void connected(const QString&);
    void disconnected(const QString&);
    void error(const QString&);

public:
    Tcp_socket();
    ~Tcp_socket() override;
    void SendMsg(const ProtosMessage&);
    void Connect(const QString &Ip, const int &Port);
    void Disconnect();
    std::optional<QString> GetIp();
    int  GetPort() const;
    bool IsConnected() const;
    void AddRxMsgHandler(const MsgHandlerT&);
    void AddTxMsgHandler(const MsgHandlerT&);
    void AddErrorHandler(const ErrorHandlerT&);

private:
    const int kServiceBytesCnt = 4,
        kMaxProtosMsgLength = 15,
        kMinPacketLength = 9;
    bool connected_ = false;
    QMutex mutex_;
    QString ip_;
    int port_{};

    MsgQueue tx_msg_queue_;

    std::atomic<bool> thread_flag_{true};
    QSharedPointer<QThread> socket_thread_;

    QList<MsgHandlerT> rx_msg_handler_list_;
    QList<MsgHandlerT> tx_msg_handler_list_;
    QList<ErrorHandlerT> error_handler_list_;

    QByteArray data_buffer_{};

    void ReadingLoop();
    void MsgPullOut(QByteArray&);
    QByteArray PackSocketMsg(ProtosMessage &msg);
    static std::pair<QString, ProtosMessage> ConvertDataToMsg(const QByteArray&);

    void ErrorHandler(const QString&);
    void RxMsgHandler(ProtosMessage&);
    void TxMsgHandler(const ProtosMessage&);
    void ThreadsDelete();

    enum class MsgByteType
    {
        START_BYTE,
        FIRST_STATUS_BYTE,
        SECOND_STATUS_BYTE,
        DATA_BYTE,
        STOP_BYTE
    };
};