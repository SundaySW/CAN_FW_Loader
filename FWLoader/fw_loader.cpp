//
// Created by outlaw on 21.06.2022.
//

#include "fw_loader.hpp"

FWLoader::FWLoader()
{}

void FWLoader::addDevice(const QString &fileName, uchar addr, uint uid, uchar uidT, uchar ver) {
    DeviceHolder device = DeviceHolder(fileName, addr, uid, uidT, ver);
    device.OnNextBlockSignal = [this](uint delta, uint uid, uint addr){ signalNextBlock(delta, uid, addr); };
    device.readyToSendSignal = [this](uint UID){ signalBootData(UID); };
    device.errorSignal       = [this](const QString& error, uint uid){ signalError(error, uid); };
    device.finishedDevice    = [this](uint uid, int msecs){removeDevice(uid, msecs);};
    deviceList.insert(uid, device);
    device.sendJumpToBootmsg();
}

void FWLoader::transmitBlocks() {
    for(auto& device: deviceList)
        device.transmitBlock();
}

void FWLoader::ParseBootMsg(const ProtosMessage& msg) {
    if ((msg.ProtocolType != ProtosMessage::RAW) || !msg.Dlc || (msg.BootLoader != ProtosMessage::BOOT))
        return;
    uint uid = (msg.IdBytes[2] << 16) | (msg.IdBytes[1] << 8) | msg.IdBytes[0];
    if(deviceList.contains(uid)){
        DeviceHolder *device = &deviceList[uid];
        uchar messageType = msg.BootMsgType;
        if (messageType == Protos::MSGTYPE_BOOT_FLOW) {
            uchar FcFlag = msg.Data[1];
            uchar FcCode = msg.Data[2];
            if (FcFlag == Protos::BOOT_FC_FLAG_FC){
                uint16_t onTargetBlockNum   = msg.Data[6] + (msg.Data[7] << 8);
                uint16_t missed_from        = msg.Data[3] + (msg.Data[4] << 8);
                switch (FcCode) {
                    case Protos::BOOT_FC_RESEND_PACKETS:
                        device->missedPackets(missed_from, msg.Data[5], onTargetBlockNum);
                        break;
                    case Protos::BOOT_FC_BLOCK_UNVALIDATED:
                    case Protos::BOOT_FC_BLOCK_OK:
                    case Protos::BOOT_FC_FLASH_BLOCK_WRITE_FAIL:
                    case Protos::BOOT_FC_BLOCK_CRC_FAIL:
                        device->manageBlock(onTargetBlockNum);
                        break;
                    case Protos::BOOT_FC_FLASH_NOT_READY:
                        device->restart();
                        break;
                    case Protos::BOOT_FC_FLASH_READY:
                    default:
                        break;
                }
            }
        }
        else if (messageType == Protos::MSGTYPE_BOOT_ACK) {
            if(device->isLastBlock())
                device->finishDevice();
            signalAckReceived();
            device->ackReceived();
        }
    }
}

void FWLoader::transmitBlock(uint uid) {
    if(deviceList.contains(uid))
        deviceList[uid].transmitBlock();
}

void FWLoader::cancelFWLoad(uint uid) {
    if(deviceList.contains(uid)) {
        deviceList[uid].finishProcess();
        deviceList.remove(uid);
    }
}

void FWLoader::removeDevice(uint uid, int msecs) {
    if(deviceList.contains(uid)) {
        deviceList.find(uid)->finishProcess();
        deviceList.remove(uid);
        signalFinishedOK(uid, msecs);
    }
}