#ifndef PROTOS_H_
#define PROTOS_H_

namespace Protos
{
	enum
	{
		BROADCAST = 0xFF
	};

	enum PARAM_FIELD
	{
		FIELD_ID,
		FIELD_TYPE,
		FIELD_VALUE,
		FIELD_NAME,				// NOT USED
		FIELD_SENDRATE,
		FIELD_READONLY,
		FIELD_SENDTIME,
		FIELD_UPDATEBEFOREREAD, // NOT USED
		FIELD_UPDATERATE,
		FIELD_UPDATETIME,
		FIELD_UPDATABLE,
		FIELD_VALUETOGO,
		FIELD_CTRLBIT,
		FIELD_CTRLRATE,
		FIELD_CTRLTIME,
		FIELD_MULT,				///< multiplier for value calibration
		FILED_OFFSET,			///< offset for value calibration
		FIELD_ID_EXT,			///< extension for id
		FIELD_SPECIAL			///
	};

	enum PSF_returns
	{
		RET_PSF_OK,
		RET_PSF_NOID,
		RET_PSF_NOFIELD,
		RET_PSF_RO,
		RET_PSF_NOT_UPDATABLE,
		RET_PSF_WRONGTYPE,
		RET_PSF_NOT_CALIBRATEABLE
		//Ret_PSF_NotCTRL
	};

	enum MISC_CMD
	{
		MISC_CMD_WRITE_EEPROM    = 0x20,
		MISC_CMD_READ_EEPROM     = 0x21,
		MISC_CMD_ENUM_ALL_PARAMS = 0x40
	};

	// Message types
	enum MSG_TYPE
	{
		MSGTYPE_NULL				= 0x0,

		MSGTYPE_CMDMISC			= 0x01, ///< ����� ������� ��� ���� ���������
		MSGTYPE_CMDSPEC			= 0x02, ///< ��������� � ������� ���������� ������� ��� ������������ ���� ����������
		MSGTYPE_CMDMISC_ANSWER  = 0x03, ///< ����� �� ������� CMDMISC
		MSGTYPE_CMDSPEC_ANSWER  = 0x04, ///< ��������� � ������� �� ��������� ���� MSGTYPE_CMDSPEC
		MSGTYPE_CMDMISC_ERROR   = 0x05, ///< ������ ��� ������������� ���������� ������� ���� CMD_MISC
		MSGTYPE_CMDSPEC_ERROR   = 0x06, ///< ������ ��� ������������� ���������� ������� ���� CMD_SPEC
		MSGTYPE_PARAM_REQUEST   = 0x07, ///< ������ ����\�������� � ���������
		MSGTYPE_PARAM_ANSWER    = 0x08, ///< ����� ���������� �� ������
		MSGTYPE_PARAM_SET	    = 0x09, ///< ������� ����\�������� ���������
		MSGTYPE_PARAM_ERROR     = 0x0A, ///< ������ ��� ������������� ���������� ������� set\request
		MSGTYPE_EVENT			= 0x0B, ///< ��������� � ��������� �������
	};

	enum MSG_TYPE_BOOT
	{
		MSGTYPE_BOOT_DATA		= (0x00),
		MSGTYPE_BOOT_ADDR_CRC	= (0x01),
		MSGTYPE_BOOT_ACK		= (0x05),
		MSGTYPE_BOOT_BOOTREQ	= (0x02),
		MSGTYPE_BOOT_FLOW		= (0x07),
	};

    enum BOOT_FC_FLAG{
        BOOT_FC_FLAG_ACK = 0x1,
        BOOT_FC_FLAG_FC = 0x2,
        BOOT_FC_FLAG_ERROR = 0x3,
    };

	enum BOOT_FC_CODE{
        BOOT_FC_BLOCK_OK = 0x1,
        BOOT_FC_RESEND_PACKETS = 0x2,
        BOOT_FC_FINISH_FLASH = 0x3,
        BOOT_FC_FLASH_NOT_READY = 0x4,
        BOOT_FC_FLASH_BLOCK_WRITE_FAIL = 0x5,
        BOOT_FC_STAY_IN_BOOT = 0x6,
        BOOT_FC_BLOCK_UNVALIDATED = 0x7,
        BOOT_FC_FLASH_READY = 0x8,
        BOOT_FC_BLOCK_CRC_FAIL = 0x9,
    };

	// Flow control flag
	enum FC_FLAG
	{
		FC_ACCEPT = 1,
		FC_WAIT   = 2,
		FC_ABORT  = 3
	};

	// Short/long packet flag
	enum SL_FLAG
	{
		SL_SHORT = 0,
		SL_LONG_FIRST,
		SL_LONG_NEXT,
		SL_FLOW_CTRL
	};

}//namespace Protos

#endif /* PROTOS_H_ */
