
#include <QTcpSocket>
#include <utility>
#include "SocketThread.hpp"
#include "QDataStream"

SocketThread::SocketThread(QString IP, qint16 port, QObject *parent)
    : QThread(parent),
    ip_(std::move(IP)),
    port_(port)
{}

void SocketThread::run(){
    QTcpSocket socket = QTcpSocket(this);
    socket.connectToHost(ip_, port_);
    if(!socket.waitForConnected()){
        emit error(QString("Connection Error Ip:%1 port:%2").arg(ip_).arg(port_));
        return;
    }else
        emit statusUpdate(QString("Connected to %1:%2").arg(socket.peerName()).arg(socket.peerPort()));

    QByteArray data_to_send;

    while(quit_.load()){
        if(socket.waitForReadyRead()){
            auto socket_data = socket.readAll();
            MsgPullOut(socket_data);
        }
        while(tx_msg_queue_.try_dequeue(data_to_send)){
            auto bytes_written = socket.write(data_to_send);
            bool result = bytes_written == data_to_send.size();
            if(!result)
                emit error("Error while writing to socket");
        }
    }
}

SocketThread::~SocketThread()
{
    quit_.store(false);
//    cv_.wakeOne();
    wait();
}

void SocketThread::PlaceMsg(ProtosMessage& msg) {
    auto data = PackSocketMsg(msg);
    tx_msg_queue_.enqueue(data);
}

QByteArray SocketThread::PackSocketMsg(ProtosMessage& msg)
{
    int bytes_to_send = msg.Dlc + ProtosMessage::IdLng;
    if (bytes_to_send <= kMaxProtosMsgLength)
    {
        QByteArray data(bytes_to_send + kServiceBytesCnt, '0');
        ushort dataIdx = 0;
        data[dataIdx++] = '#';
        data[dataIdx++] = 0x40 + (0x3f & (bytes_to_send >> 8));
        data[dataIdx++] = char(bytes_to_send & 0xff);
        for (std::size_t msgIdx = 0; msgIdx < bytes_to_send; msgIdx++)
            data[dataIdx++] = char(msg[msgIdx]);
        data[dataIdx] = '\r';
        return data;
    }
    else
    {
        emit error(QString("Try to send message longer (%1 bytes) "
                             "than maximum PROTOS message length (%2 bytes).")
                             .arg(bytes_to_send).arg(kMaxProtosMsgLength));
        return {};
    }
}

void SocketThread::MsgPullOut(QByteArray& socket_data) {
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
                tx_msg_queue_.enqueue(data);
            else if (msg.first != "0")
                emit error(msg.first);
        }
    }
}

std::pair<QString, ProtosMessage> SocketThread::ConvertDataToMsg(const QByteArray& data){
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
