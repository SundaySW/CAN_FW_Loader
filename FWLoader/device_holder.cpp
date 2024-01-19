
#include "device_holder.hpp"

DeviceHolder::DeviceHolder(const QString &fileName, uchar address, uint uid, uchar uidT, uchar ver):
        Address(address)
        , UID(uid)
        , UIDType(uidT)
        , dataPending(false)
        , loadingSWVer(ver)
{
    BABuffer = new QByteArray(BLOCK_SIZE_FLASH, 0xFF);
    fwFile = new QFile(fileName);
    fwFile->open(QIODevice::ReadWrite);
    fileDataStream = new QDataStream(fwFile);
    qint64 fileSize = fileDataStream->device()->size();
    totalBlocks = fileSize / BLOCK_SIZE_FLASH + ((fileSize % BLOCK_SIZE_FLASH) ? 1 : 0);
}

void DeviceHolder::processValidatedData(){
    sendDataPackets(BABuffer->size());
}

bool DeviceHolder::transmitBlock(){
    bool result = true;
    if(dataPending){
        dataPending = false;
        qint32 bytes = fileDataStream->readRawData(BABuffer->data(), BABuffer->size());
        BABuffer->resize(bytes);
        sendAddrCRCmsg(bytes);
//        sendDataPackets(bytes);
    }
    return result;
}

void DeviceHolder::manageBlock(uint receivedBlockNum) {
    processBlock(receivedBlockNum);
}

void DeviceHolder::processBlock(uint blockNum) {
    if(setBlockSeekFile(blockNum)){
        OnNextBlockSignal(getStatusBarData(), UID, Address);
        dataPending = true;
//        readyToSendSignal(UID);
        transmitBlock();
    }
    else{
        if(blockNum == (totalBlocks))
            finishDevice();
        else errorSignal("Target sent blockNum out of range", UID);
    }
}

inline bool DeviceHolder::setBlockSeekFile(uint16_t targetBlockNum, int nOfPackets, int blockOffsetInPackets){
    bool result = false;
    if(targetBlockNum < totalBlocks && targetBlockNum >= 0) {
        currentBlock = targetBlockNum;
        result = fileDataStream->device()->seek((currentBlock * BLOCK_SIZE_FLASH) + (blockOffsetInPackets * BYTES_IN_PACKET));
        BABuffer->resize(nOfPackets * BYTES_IN_PACKET);
    }
    return result;
}

void DeviceHolder::missedPackets(uint16_t from, uint8_t len, uint16_t targetBlockNum){
    setBlockSeekFile(targetBlockNum, len, from);
    dataPending = true;
//    readyToSendSignal(UID);
    transmitBlock();
}

void DeviceHolder::ackReceived(){
    elapsedTimer.start();
    SendStayInBootMsg();
    processBlock(0);
}

inline uint16_t DeviceHolder::calcCRC(int dataLen) {
    uint16_t currentCRC = 0;
    for(uint8_t x : *BABuffer)
        currentCRC += x;
    currentCRC = (((~currentCRC) + 1) & 0xffff);
    return currentCRC;
}

void DeviceHolder::sendDataPackets(int len){
    uint32_t absByteNum = fileDataStream->device()->pos();
    int BABufferSize = BABuffer->size();
    if (BABufferSize >= 0)
        absByteNum = absByteNum - BABufferSize;
    uint8_t bufferPacketData[8];
    uint packetNum = 0;
    for (int i = 0; i < len; ++i) {
        uint index = i % BYTES_IN_PACKET;
        bufferPacketData[index] = BABuffer->at(i);
        if(!((i+1) % BYTES_IN_PACKET)){
            sendDatamsg(bufferPacketData, absByteNum + (packetNum * BYTES_IN_PACKET));
            packetNum++;
        }
    }
    int incompletePacketFirstIndex = len % BYTES_IN_PACKET;
    if(incompletePacketFirstIndex){
        for(int i = incompletePacketFirstIndex; i < BYTES_IN_PACKET; i++)
            bufferPacketData[i] = 0xFF;
        sendDatamsg(bufferPacketData, absByteNum + (packetNum * BYTES_IN_PACKET));
    }
}

void DeviceHolder::sendFinishFlashMsg(){
    uchar data[8];
    data[0] = UIDType;
    data[1] = Address;
    data[2] = Protos::BOOT_FC_FLAG_FC;
    data[3] = Protos::BOOT_FC_FINISH_FLASH;
    sendBootmsg(data, UID, Protos::MSGTYPE_BOOT_FLOW);
}

uint DeviceHolder::getStatusBarData() const{
    return int(float(currentBlock)/float(totalBlocks)*100);
}

void DeviceHolder::restart(){
    processBlock(0);
}

void DeviceHolder::finishProcess(){
    fwFile->close();
    BABuffer->clear();
}

void DeviceHolder::sendDatamsg(uint8_t* data, uint absByteNum){
    sendBootmsg(data, absByteNum,Protos::MSGTYPE_BOOT_DATA);
}

void DeviceHolder::sendJumpToBootmsg(){
    uchar data[8];
    data[0] = UIDType;
    data[1] = Address;
    sendBootmsg(data, UID, Protos::MSGTYPE_BOOT_BOOTREQ);
}

void DeviceHolder::SendStayInBootMsg(){
    uchar data[8];
    data[0] = UIDType;
    data[1] = Address;
    data[2] = Protos::BOOT_FC_FLAG_FC;
    data[3] = Protos::BOOT_FC_STAY_IN_BOOT;
    data[4] = totalBlocks & 0xff;
    data[5] = (totalBlocks >> 8) & 0xff;
    data[6] = loadingSWVer;
    sendBootmsg(data, UID, Protos::MSGTYPE_BOOT_FLOW);
}

void DeviceHolder::sendAddrCRCmsg(int dataLen){
    uint32_t absByteNum = fileDataStream->device()->pos();
    int BABufferSize = BABuffer->size();
    if (BABufferSize >= 0)
        absByteNum = absByteNum - BABufferSize;
    uint16_t CRC = calcCRC(dataLen);
    uchar data[8];
    data[0] = CRC & 0xff;
    data[1] = (CRC >> 8) & 0xff;
    data[2] = dataLen & 0xff;
    data[3] = (dataLen >> 8) & 0xff;
    data[7] = Address;
    sendBootmsg(data, absByteNum, Protos::MSGTYPE_BOOT_ADDR_CRC);
}

void DeviceHolder::sendBootmsg(uchar* data, uint32_t idBytes, uchar msgType){
    ProtosMessage setAddrCRCMsg(0, 0, ProtosMessage::MsgTypes::NONE, 8,
                  data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
    setAddrCRCMsg.IdBytes[0] = idBytes & 0xff;
    setAddrCRCMsg.IdBytes[1] = (idBytes >> 8) & 0xff;
    setAddrCRCMsg.IdBytes[2] = (idBytes >> 16) & 0xff;
    setAddrCRCMsg.IdBytes[3] = 0;
    setAddrCRCMsg.BootMsgType = msgType;
    setAddrCRCMsg.ProtocolType = ProtosMessage::RAW;
    setAddrCRCMsg.BootLoader = ProtosMessage::BOOT;

    msgToSend(setAddrCRCMsg);
}

bool DeviceHolder::isLastBlock() {
    return (currentBlock == (totalBlocks - 1));
}

void DeviceHolder::finishDevice() {
    sendFinishFlashMsg();
    OnNextBlockSignal(100, UID, Address);
    finishedDevice(UID, elapsedTimer.elapsed());
}