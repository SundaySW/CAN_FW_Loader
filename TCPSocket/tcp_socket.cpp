
#include "tcp_socket.hpp"

#include <utility>
#include "QDebug"


Tcp_socket::Tcp_socket() {
    read_thread_ = QThread::create([this](){ ReadingLoop(); });
    write_thread_ = QThread::create([this](){ WritingLoop(); });
    read_thread_->start();
    write_thread_->start();
}

void Tcp_socket::SendMsg(const ProtosMessage& msg)
{
    tx_msg_queue_.enqueue(msg);
}

void Tcp_socket::ReadingLoop(){
    while(read_thread_flag_.load()){
        if(socket_.canReadLine())
            MsgPullOut2();
    }
}

void Tcp_socket::WritingLoop(){
    while(write_thread_flag_.load()){
        ProtosMessage msg;
        if(tx_msg_queue_.try_dequeue(msg)){
            if(!WriteMsg(msg))
                ErrorHandler("Error while writing to socket");
            else
                TxMsgHandler(msg);
        }
    }
}

bool Tcp_socket::WriteMsg(ProtosMessage& msg)
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
            data[dataIdx++] = msg[msgIdx];
        data[dataIdx] = '\r';
        return (socket_.write(data) == bytes_to_send + kServiceBytesCnt);
    }
    else
    {
        ErrorHandler(QString("Try to send message longer (%1 bytes) "
                             "than maximum PROTOS message length (%2 bytes).")
                             .arg(bytes_to_send).arg(kMaxProtosMsgLength));
        return false;
    }
}

void Tcp_socket::MsgPullOut2() {
    char byte;
    int msg_type, msg_length, idx;
    ProtosMessage msg;
    while (socket_.read(&byte, 1) == 1)
    {
        switch (expected_byte_)
        {
            case START_BYTE:
                if (byte == '#')
                    expected_byte_ = FIRST_STATUS_BYTE;
                break;
            case FIRST_STATUS_BYTE:
                msg_type = byte & 0xc0;
                msg_length = byte & 0x3f;
                msg_length <<= 8;
                msg_length &= 0x3f00;
                expected_byte_ = SECOND_STATUS_BYTE;
                break;
            case SECOND_STATUS_BYTE:
                expected_byte_ = DATA_BYTE;
                msg_length |= byte;
                msg.Dlc = msg_length - ProtosMessage::IdLng;
                idx = 0;
                break;
            case DATA_BYTE:
                msg[idx++] = byte;
                if (idx == msg_length)
                    expected_byte_ = STOP_BYTE;
                break;
            case STOP_BYTE:
                expected_byte_ = START_BYTE;
                if (byte == '\r' || byte == '\n')
                    RxMsgHandler(msg);
                else
                    ErrorHandler("Unexpected msg end");
                break;
        }
    }
}

void Tcp_socket::MsgPullOut() {
    auto data = socket_.readLine();
    if(data.isEmpty()){
        ErrorHandler("Emtpy byte array received after readyRead signal");
        return;
    }
    if(!data.contains('#')){
        ErrorHandler("Received byte array without correct start byte");
        return;
    }
    auto msg = ConvertDataToMsg(data);
    if(msg.second.Dlc)
        RxMsgHandler(msg.second);
    else
        ErrorHandler(msg.first);
}

std::pair<QString, ProtosMessage> Tcp_socket::ConvertDataToMsg(const QByteArray& data){
    int msg_type, msg_length;
    ProtosMessage msg;

    auto idx = data.indexOf("#");
    if(data.size() < idx + 2)
        return {"Incorrect msg format, status section miss", msg};

    std::pair<char, char> status {data.at(++idx), data.at(++idx)};
    msg_type = status.first & 0xc0;
    msg_length = status.first & 0x3f;
    msg_length <<= 8;
    msg_length &= 0x3f00;
    msg_length |= status.second;
    msg.Dlc = msg_length - ProtosMessage::IdLng;

    if(data.size() < (idx + msg.Dlc))
        return {"Incorrect msg format, data section miss", msg};

    for (std::size_t i = 0; i < msg.Dlc; i++)
        msg[i] = data.at(++idx);

    if (++idx == data.size())
        return {"0", msg};
    else
        return {"Incorrect msg end", msg};
}

void Tcp_socket::ErrorHandler(const QString& errSt)
{
    emit error(errSt);
    for (const auto& handler : error_handler_list_)
        handler(errSt);
}

bool Tcp_socket::Connect(const QString &Ip, const int &Port, int Timeout) {
    if (IsConnected())
        Disconnect();
    socket_.connectToHost(Ip, Port);
    socket_.waitForConnected(Timeout);
    if (IsConnected())
        return true;
    else
        return false;
}

bool Tcp_socket::IsConnected() {
    return socket_.state() == QAbstractSocket::ConnectedState;
}

void Tcp_socket::Disconnect(int Timeout) {
    socket_.disconnectFromHost();
    socket_.waitForDisconnected(Timeout);
}

std::optional<QString> Tcp_socket::GetIp() {
    if(IsConnected())
        return socket_.peerName();
    else return {};
}

int Tcp_socket::GetPort() {
    return IsConnected() ? socket_.peerPort() : -1;
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
    msg.SenderAddr = 0x99;
    SendMsg(msg);
    for (const auto& handler : rx_msg_handler_list_)
        handler(msg);
}

void Tcp_socket::TxMsgHandler(const ProtosMessage& msg) {
    for(const auto& handler : tx_msg_handler_list_)
        handler(msg);
}

Tcp_socket::~Tcp_socket() {
    read_thread_flag_.store(false);
    write_thread_flag_.store(false);
    read_thread_->deleteLater();
    write_thread_->deleteLater();
}
