//
// Created by outlaw on 21.06.2022.
//

#include "fw_loader.hpp"

FWLoader::FWLoader(const QSharedPointer<Tcp_socket>& socket):
    socket_(socket)
{}

void FWLoader::addDevice(const QString &fileName, std::optional<uchar> addr, uint uid, uchar uidT, uchar ver) {
    DeviceHolder device = DeviceHolder(fileName, addr, uid, uidT, ver);
    device.OnNextBlockSignal = [this](uint delta, uint uid, uint addr)  { signalNextBlock(delta, uid, addr); };
    device.ErrorSignal       = [this](const QString& error, uint uid)   { signalError(error, uid); };
    device.DeviceFinished    = [this](uint uid, qint64 msecs)           { FinishDevice(uid, msecs);};
    device.SendMsg           = [this](const ProtosMessage& msg)         { socket_->SendMsg(msg); };
    device_list_.insert(uid, device);
    device.SendJumpToBootMsg();
}

void FWLoader::ParseBootMsg(const ProtosMessage& msg) {
    if ((msg.ProtocolType != ProtosMessage::RAW) || !msg.Dlc || (msg.BootLoader != ProtosMessage::BOOT))
        return;
    uint uid = (msg.IdBytes[2] << 16) | (msg.IdBytes[1] << 8) | msg.IdBytes[0];
    if(device_list_.contains(uid)){
        DeviceHolder *device = &device_list_.find(uid).value();
        uchar message_type = msg.BootMsgType;
        if (message_type == Protos::MSGTYPE_BOOT_FLOW) {
            uchar FcFlag = msg.Data[1];
            uchar FcCode = msg.Data[2];
            if (FcFlag == Protos::BOOT_FC_FLAG_FC){
                uint16_t device_block_n   = msg.Data[6] + (msg.Data[7] << 8);
                uint16_t missed_from        = msg.Data[3] + (msg.Data[4] << 8);
                uint8_t resend_packets_n    = msg.Data[5];
                switch (FcCode) {
                    case Protos::BOOT_FC_BLOCK_OK:
                        device->LoadNextBlock(device_block_n);
                        break;
                    case Protos::BOOT_FC_RESEND_PACKETS:
                        device->ProcessMissedPackets(missed_from, resend_packets_n, device_block_n);
                        break;
                    case Protos::BOOT_FC_BLOCK_UNVALIDATED:
                        device->ManageBlock(device_block_n);
                        break;
                    case Protos::BOOT_FC_FLASH_BLOCK_WRITE_FAIL:
                        device->ManageBlock(device_block_n);
                        break;
                    case Protos::BOOT_FC_BLOCK_CRC_FAIL:
                        device->ManageBlock(device_block_n);
                        break;
                    case Protos::BOOT_FC_FLASH_NOT_READY:
                        device->RestartProcess();
                        break;
                    case Protos::BOOT_FC_FLASH_READY:
                    default:
                        break;
                }
            }
        }
        else if (message_type == Protos::MSGTYPE_BOOT_ACK){
            auto addr =     msg.Data[1];
            auto HWVer =    msg.Data[3];
            auto FWVer =    msg.Data[4];
            signalAckReceived(uid, addr, HWVer, FWVer);
            device->AckReceived(addr);
        }
    }
}

void FWLoader::RefreshDevice(uint uid) {
    if (device_list_.contains(uid))
    {
        auto device = device_list_.find(uid).value();
        device.IsFirstBlock() ? device.StartUpload() :
                                device.RefreshUpload();
    }
}

void FWLoader::CancelFWLoad(uint uid) {
    if(device_list_.contains(uid)) {
        device_list_.find(uid).value().SendExitBoot();
        device_list_.find(uid).value().FinishProcess();
        device_list_.remove(uid);
    }
}

void FWLoader::FinishDevice(uint uid, qint64 msecs) {
    if(device_list_.contains(uid)) {
        signalFinishedOK(uid, msecs);
        device_list_.find(uid).value().FinishProcess();
        device_list_.remove(uid);
    }
}