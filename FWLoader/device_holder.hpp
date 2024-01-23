#pragma once

#include <QtCore/QString>
#include <QtCore/QFile>
#include "QDataStream"
#include <QtCore/QElapsedTimer>

#include <protos_message.h>
#include <Protos/protos.h>

#include "TCPSocket/tcp_socket.hpp"

#define BLOCK_SIZE_FLASH    (4096)
#define BYTES_IN_PACKET     (8)
#define PACKETS_IN_BLOCK    (BLOCK_SIZE_FLASH/BYTES_IN_PACKET)

class DeviceHolder{
public:
    std::function<void(uint delta, uint uid, uint addr)> OnNextBlockSignal;
    std::function<void(const QString& error, uint uid)> ErrorSignal;
    std::function<void(uint uid, qint64 msecs)> DeviceFinished;
    std::function<void(const ProtosMessage&)> SendMsg;

    DeviceHolder() = delete;
    DeviceHolder(const QString&, uchar, uint, uchar, uchar);
    void TransmitBlock();
    void AckReceived();
    void ProcessMissedPackets(uint16_t from, uint8_t len, uint16_t targetBlockNum);
    [[nodiscard]] bool IsLastBlock() const;
    [[nodiscard]] bool IsFirstBlock() const;
    void ManageBlock(uint receivedBlockNum);
    [[nodiscard]] uint GetStatusBarData() const;
    void FinishProcess();
    void RestartProcess();
    void SendJumpToBootMsg();
    void SendExitBoot();
    void StartUpload();
    void RefreshUpload();

protected:
private:
    uchar address_ = 0;
    uint uid_ = 0;
    uchar uid_type_ = 0;
    uint current_block_ = 0;
    uint blocks_total_ = 0;
    QFile* fw_file_;
    QByteArray* buffer_;
    QDataStream* file_stream_;
    bool data_pending_;
    QElapsedTimer elapsed_timer_;
    uchar loading_sw_ver_ = 0;

    bool SetBlock(uint16_t targetBlockNum, int nOfPackets = PACKETS_IN_BLOCK, int blockOffsetInPackets = 0);
    uint16_t CalcCRC();
    void SendDataPackets(int len);
    void SendDataMsg(uint8_t *data, uint absByteNum);
    void SendBootMsg(uchar *data, uint32_t idBytes, uchar msgType) const;
    void SendAddrCRCMsg(int dataLen);
    void SendExitBootMsg();
    void SendStayInBootMsg();
    void ProcessBlock(uint blockNum);
    void FinishDevice();
};
