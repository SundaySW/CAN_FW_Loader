
#include "device_holder.hpp"

DeviceHolder::DeviceHolder(const QString &fileName, std::optional<uchar> addr, uint uid, uchar uidT, uchar ver):
        uid_(uid)
        , uid_type_(uidT)
        , data_pending_(false)
        , loading_sw_ver_(ver)
{
    if(!addr.has_value())
        address_ = DEFAULT_DEVICE_ADDR;
    else
        address_ = addr.value();
    buffer_ = new QByteArray(BLOCK_SIZE_FLASH, static_cast<char>(0xFF));
    fw_file_ = new QFile(fileName);
    fw_file_->open(QIODevice::ReadWrite);
    file_stream_ = new QDataStream(fw_file_);
    qint64 fileSize = file_stream_->device()->size();
    blocks_total_ = fileSize / BLOCK_SIZE_FLASH + ((fileSize % BLOCK_SIZE_FLASH) ? 1 : 0);
}

void DeviceHolder::TransmitBlock(){
    if(data_pending_){
        data_pending_ = false;
        qint32 bytes = file_stream_->readRawData(buffer_->data(), buffer_->size());
        buffer_->resize(bytes);
        SendAddrCRCMsg(bytes);
        SendDataPackets(bytes);
    }
}

void DeviceHolder::ManageBlock(uint receivedBlockNum){
    ProcessBlock(receivedBlockNum);
}

void DeviceHolder::LoadNextBlock(uint receivedBlockNum){
    if(current_block_ == 0)
        elapsed_timer_.start();
    ProcessBlock(receivedBlockNum);
}

void DeviceHolder::StartUpload() {
    ProcessBlock(0);
}

void DeviceHolder::RefreshUpload() {
    ProcessBlock(current_block_);
}

void DeviceHolder::ProcessBlock(uint blockNum) {
    if(SetBlock(blockNum)){
        OnNextBlockSignal(GetStatusBarData(), uid_, address_.value());
        data_pending_ = true;
        TransmitBlock();
    }
    else{
        if(blockNum == (blocks_total_))
            FinishDevice();
        else ErrorSignal("Target sent blockNum out of range", uid_);
    }
}

inline bool DeviceHolder::SetBlock(uint16_t targetBlockNum, int nOfPackets, int blockOffsetInPackets){
    bool result = false;
    if(targetBlockNum < blocks_total_ && targetBlockNum >= 0) {
        current_block_ = targetBlockNum;
        result = file_stream_->device()->seek((current_block_ * BLOCK_SIZE_FLASH) + (blockOffsetInPackets * BYTES_IN_PACKET));
        buffer_->resize(nOfPackets * BYTES_IN_PACKET);
    }
    return result;
}

void DeviceHolder::ProcessMissedPackets(uint16_t from, uint8_t len, uint16_t targetBlockNum){
    SetBlock(targetBlockNum, len, from);
    data_pending_ = true;
    TransmitBlock();
}

void DeviceHolder::AckReceived(uchar device_addr){
    if(address_.value() == DEFAULT_DEVICE_ADDR)
        address_ = device_addr;
    SendStayInBootMsg();
}

inline uint16_t DeviceHolder::CalcCRC() {
    uint16_t currentCRC = 0;
    for(uint8_t x : *buffer_)
        currentCRC += x;
    currentCRC = (((~currentCRC) + 1) & 0xffff);
    return currentCRC;
}

void DeviceHolder::SendDataPackets(int len){
    uint32_t absByteNum = file_stream_->device()->pos();
    int BABufferSize = buffer_->size();
    if (BABufferSize >= 0)
        absByteNum = absByteNum - BABufferSize;
    uint8_t bufferPacketData[8];
    uint packetNum = 0;
    for (int i = 0; i < len; ++i) {
        uint index = i % BYTES_IN_PACKET;
        bufferPacketData[index] = buffer_->at(i);
        if(!((i+1) % BYTES_IN_PACKET)){
            SendDataMsg(bufferPacketData, absByteNum + (packetNum * BYTES_IN_PACKET));
            packetNum++;
        }
    }
    int incompletePacketFirstIndex = len % BYTES_IN_PACKET;
    if(incompletePacketFirstIndex){
        for(int i = incompletePacketFirstIndex; i < BYTES_IN_PACKET; i++)
            bufferPacketData[i] = 0xFF;
        SendDataMsg(bufferPacketData, absByteNum + (packetNum * BYTES_IN_PACKET));
    }
}

void DeviceHolder::SendExitBootMsg(){
    uchar data[8];
    data[0] = uid_type_;
    data[1] = address_.value();
    data[2] = Protos::BOOT_FC_FLAG_FC;
    data[3] = Protos::BOOT_FC_EXIT_BOOT;
    SendBootMsg(data, uid_, Protos::MSGTYPE_BOOT_FLOW);
}

uint DeviceHolder::GetStatusBarData() const{
    return int(float(current_block_) / float(blocks_total_) * 100);
}

void DeviceHolder::RestartProcess(){
    ProcessBlock(0);
}

void DeviceHolder::FinishProcess(){
    fw_file_->close();
    buffer_->clear();
}

void DeviceHolder::SendDataMsg(uint8_t* data, uint absByteNum){
    SendBootMsg(data, absByteNum, Protos::MSGTYPE_BOOT_DATA);
}

void DeviceHolder::SendJumpToBootMsg(){
    uchar data[8];
    data[0] = uid_type_;
    data[1] = address_.value();
    SendBootMsg(data, uid_, Protos::MSGTYPE_BOOT_BOOTREQ);
}

void DeviceHolder::SendStayInBootMsg(){
    uchar data[8];
    data[0] = uid_type_;
    data[1] = address_.value();
    data[2] = Protos::BOOT_FC_FLAG_FC;
    data[3] = Protos::BOOT_FC_STAY_IN_BOOT;
    data[4] = blocks_total_ & 0xff;
    data[5] = (blocks_total_ >> 8) & 0xff;
    data[6] = loading_sw_ver_;
    SendBootMsg(data, uid_, Protos::MSGTYPE_BOOT_FLOW);
}

void DeviceHolder::SendAddrCRCMsg(int dataLen){
    uint32_t absByteNum = file_stream_->device()->pos();
    int BABufferSize = buffer_->size();
    if (BABufferSize >= 0)
        absByteNum = absByteNum - BABufferSize;
    uint16_t CRC = CalcCRC();
    uchar data[8];
    data[0] = CRC & 0xff;
    data[1] = (CRC >> 8) & 0xff;
    data[2] = dataLen & 0xff;
    data[3] = (dataLen >> 8) & 0xff;
    data[4] = uid_ & 0xff;
    data[5] = (uid_ >> 8) & 0xff;
    data[6] = (uid_ >> 16) & 0xff;
    data[7] = address_.value();
    SendBootMsg(data, absByteNum, Protos::MSGTYPE_BOOT_ADDR_CRC);
}

void DeviceHolder::SendBootMsg(uchar* data, uint32_t idBytes, uchar msgType) const{
    ProtosMessage setAddrCRCMsg(0, 0, ProtosMessage::MsgTypes::NONE, 8,
                  data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
    setAddrCRCMsg.IdBytes[0] = idBytes & 0xff;
    setAddrCRCMsg.IdBytes[1] = (idBytes >> 8) & 0xff;
    setAddrCRCMsg.IdBytes[2] = (idBytes >> 16) & 0xff;
    setAddrCRCMsg.IdBytes[3] = 0;
    setAddrCRCMsg.BootMsgType = msgType;
    setAddrCRCMsg.ProtocolType = ProtosMessage::RAW;
    setAddrCRCMsg.BootLoader = ProtosMessage::BOOT;

    SendMsg(setAddrCRCMsg);
}

bool DeviceHolder::IsLastBlock() const {
    return (current_block_ == (blocks_total_ - 1));
}

bool DeviceHolder::IsFirstBlock() const {
    return (current_block_ == 0);
}

void DeviceHolder::SendExitBoot(){
    SendExitBootMsg();
}

void DeviceHolder::FinishDevice() {
    auto ms = elapsed_timer_.elapsed();
    SendExitBootMsg();
    DeviceFinished(uid_, ms);
}