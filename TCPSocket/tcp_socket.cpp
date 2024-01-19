
#include "tcp_socket.hpp"

#include <utility>
#include <QTimer>
#include <QEventLoop>

Tcp_socket::Tcp_socket() = default;

void Tcp_socket::SendMsg(const ProtosMessage& msg)
{
    tx_msg_queue_.enqueue(msg);
}

void Tcp_socket::ReadingLoop(){
    data_buffer_.clear();
    QString serverName = ip_;
    quint16 serverPort = port_;
    QTcpSocket socket;
    ProtosMessage msg;

    socket.connectToHost(serverName, serverPort);
    if(!socket.waitForConnected()){
        emit error(QString("Connection Error Ip:%1 port:%2").arg(serverName).arg(serverPort));
        return;
    }else
        emit connected(QString("Connected to %1:%2").arg(socket.peerName()).arg(socket.peerPort()));

    connect(&socket, &QTcpSocket::disconnected, [this, serverName, serverPort]{
        emit disconnected(QString("Disconnected from %1:%2").arg(serverName).arg(serverPort));
    });

    while(thread_flag_.load()){
        if(socket.waitForReadyRead(1)){
            auto socket_data = socket.readAll();
            MsgPullOut(socket_data);
        }
        while (tx_msg_queue_.try_dequeue(msg)){
            auto data = PackSocketMsg(msg);
            auto bytes_written = socket.write(data);
            bool result = (bytes_written == data.size());
            if(!result)
                ErrorHandler("Error while writing to socket");
            else
                TxMsgHandler(msg);
        }
    }
}

QByteArray Tcp_socket::PackSocketMsg(ProtosMessage& msg)
{
    int bytes_to_send = msg.Dlc + ProtosMessage::IdLng;
    if (bytes_to_send <= kMaxProtosMsgLength)
    {
        QByteArray data(bytes_to_send + kServiceBytesCnt, '0');
        ushort dataIdx = 0;
        data[dataIdx++] = '#';
        data[dataIdx++] = 0x40 + (0x3f & (bytes_to_send >> 8));
        data[dataIdx++] = bytes_to_send & 0xff;
        for (std::size_t msgIdx = 0; msgIdx < bytes_to_send; msgIdx++)
            data[dataIdx++] = char(msg[msgIdx]);
        data[dataIdx] = '\r';
        return data;
    }
    else
    {
        ErrorHandler(QString("Try to send message longer (%1 bytes) "
                             "than maximum PROTOS message length (%2 bytes).")
                             .arg(bytes_to_send).arg(kMaxProtosMsgLength));
        return{};
    }
}

void Tcp_socket::MsgPullOut(QByteArray& socket_data){
    auto msg_list = socket_data.split('#');
    if(data_buffer_.isEmpty())
        msg_list.removeFirst();
    else{
        msg_list.first().prepend(data_buffer_);
        data_buffer_.clear();
    }
    for(auto& data : msg_list){
        if(data == msg_list.last() && data.size() < kMinPacketLength)
            data_buffer_.append(data);
        else{
            auto msg = ConvertDataToMsg(data);
            if(msg.second.Dlc)
                RxMsgHandler(msg.second);
            else if (msg.first != "0")
                ErrorHandler(msg.first);
        }
    }
}

std::pair<QString, ProtosMessage> Tcp_socket::ConvertDataToMsg(const QByteArray& data){
    ProtosMessage msg;
    int msg_length = 0;
    int cursor = 0;
    if(data.size() < (cursor + 2))
        return {"Incorrect msg format, status section miss", msg};

    std::pair<char, char> status {data.at(cursor), data.at(++cursor)};
    msg_length = status.first & 0x3f;
    msg_length <<= 8;
    msg_length |= status.second;
    msg.Dlc = msg_length - ProtosMessage::IdLng;
    if(data.size() < (cursor + msg_length + 1)) // +1 - stop byte
        return {"Incorrect msg format, data section miss", msg};

    for(std::size_t i = 0; i < msg_length; i++)
        msg[i] = data[++cursor];

    if(auto stop_byte = data[++cursor]; stop_byte == '\r' || stop_byte == '\n')
        return {"0", msg};

    return {"Incorrect msg end", msg};
}

void Tcp_socket::ErrorHandler(const QString& errSt)
{
    emit error(errSt);
    for (const auto& handler : error_handler_list_)
        handler(errSt);
}

void Tcp_socket::Connect(const QString &Ip, const int &Port, int Timeout) {
    ip_ = Ip;
    port_ = Port;
    ThreadsDelete();
    socket_thread_.reset(QThread::create([this]{ ReadingLoop(); }));
    thread_flag_.store(true);
    socket_thread_->start();
}

void Tcp_socket::Disconnect(int Timeout) {
    ThreadsDelete();
}

void Tcp_socket::ThreadsDelete(){
     if(socket_thread_){
         thread_flag_.store(false);
         socket_thread_->deleteLater();
         socket_thread_->wait();
     }
     socket_thread_.clear();
}

bool Tcp_socket::IsConnected() {
    return socket_thread_;
}

std::optional<QString> Tcp_socket::GetIp() {
    return ip_;
}

int Tcp_socket::GetPort() {
    return port_;
}

void Tcp_socket::AddRxMsgHandler(const MsgHandlerT& handler){
    rx_msg_handler_list_.append(handler);
}

void Tcp_socket::AddTxMsgHandler(const MsgHandlerT& handler){
    tx_msg_handler_list_.append(handler);
}

void Tcp_socket::AddErrorHandler(const ErrorHandlerT& handler) {
    error_handler_list_.append(handler);
}

void Tcp_socket::RxMsgHandler(ProtosMessage& msg) {
    for(const auto& handler : rx_msg_handler_list_)
        handler(msg);
}

void Tcp_socket::TxMsgHandler(const ProtosMessage& msg) {
    for(const auto& handler : tx_msg_handler_list_)
        handler(msg);
}

Tcp_socket::~Tcp_socket() {
    ThreadsDelete();
}