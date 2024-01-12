#pragma once

#include <QTcpSocket>
#include <QThread>

#include "moodycamel/concurrentqueue.h"

#include "protos_message.h"

class Tcp_socket : public QObject{
Q_OBJECT
    using MsgQueue = moodycamel::ConcurrentQueue<ProtosMessage>;
    using MsgHandlerT = std::function<void(const ProtosMessage&)>;
    using ErrorHandlerT = std::function<void(const QString&)>;
signals:
    void connected();
    void disconnected();
    void error(const QString&);

public:
    Tcp_socket();
    ~Tcp_socket();
    void SendMsg(const ProtosMessage&);
    bool Connect(const QString& Ip, const int& Port, int Timeout = 10);
    void Disconnect(int Timeout = 0);
    std::optional<QString> GetIp();
    int  GetPort();
    bool IsConnected();
    void AddRxMsgHandler(const MsgHandlerT&);
    void AddTxMsgHandler(const MsgHandlerT&);
    void AddErrorHandler(const ErrorHandlerT&);

private:
    const int kServiceBytesCnt = 4, kMaxProtosMsgLength = 15;
    QTcpSocket socket_;
    MsgQueue tx_msg_queue_;

    QThread* read_thread_;
    std::atomic<bool> read_thread_flag_{true};
    QThread* write_thread_;
    std::atomic<bool> write_thread_flag_{true};

    QList<MsgHandlerT> rx_msg_handler_list_;
    QList<MsgHandlerT> tx_msg_handler_list_;
    QList<ErrorHandlerT> error_handler_list_;

    int expected_byte_ = START_BYTE;
    enum
    {
        START_BYTE,
        FIRST_STATUS_BYTE,
        SECOND_STATUS_BYTE,
        DATA_BYTE,
        STOP_BYTE
    };

    void MsgPullOut();
    void MsgPullOut2();
    static std::pair<QString, ProtosMessage> ConvertDataToMsg(const QByteArray&);
    void ErrorHandler(const QString&);
    void ReadingLoop();
    void WritingLoop();
    bool WriteMsg(ProtosMessage&);
    void RxMsgHandler(ProtosMessage&);
    void TxMsgHandler(const ProtosMessage&);
};

