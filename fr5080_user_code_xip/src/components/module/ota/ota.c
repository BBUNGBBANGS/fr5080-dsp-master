/*
 * ota.c
 *
 *  Created on: 2018-12-27
 *      Author: Administrator
 */

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "user_def.h"
#include "crc32.h"
#include "co_mem.h"

#include "plf.h"
#include "ipc.h"

#define OTA_LOG					printf
#define FLASH_PAGE_SIZE			0x100

static struct ipc_msg_flash_op_t local_flash_op;

void qspi_flash_erase(uint32_t offset);
void qspi_flash_write(uint32_t offset, uint32_t length, uint8_t *buffer);

// 这个函数执行OTA的copy动作,将新的固件copy到运行空间,也就是会覆盖掉当前的运行区域.所以这段代码需要运行在RAM空间,包括它调用的函数也需要在RAM空间.
__attribute__((section("iram_section"))) static void ota_flash_copy(uint32_t src, uint32_t dst, uint32_t length)
{
	uint8_t *buffer = pvPortMalloc(FLASH_PAGE_SIZE);
	uint32_t sub_length;

	OTA_LOG("OTA_FLASH_COPY: src=%08x, dst=%08x, length=%08x", src, dst, length);

	GLOBAL_INT_DISABLE();
	while(length) {
		if((dst & 0x00000fff) == 0) {
			qspi_flash_erase(dst);
		}
		sub_length = length > FLASH_PAGE_SIZE ? FLASH_PAGE_SIZE : length;
		qspi_flash_read(src, sub_length, buffer);
		qspi_flash_write(dst, sub_length, buffer);
		src += sub_length;
		dst += sub_length;
		length -= sub_length;
	}

	vPortFree(buffer);

	// send response to MCU. TBD, ipc channel has to be allocated without deferment inside this function
	ipc_msg_send(IPC_MSG_WITHOUT_PAYLOAD, IPC_SUB_MSG_FLASH_COPY_ACK, NULL);

	// OTA updated, wait for reset.
	while(1);
	GLOBAL_INT_RESTORE();
}

void ota_recv_ipc_msg(uint8_t chn)
{
	uint8_t *buffer;
	struct ipc_msg_flash_op_t *flash_op;
	uint32_t length;
	uint32_t offset;

	buffer = ipc_get_buffer_offset(IPC_DIR_MCU2DSP, chn);
	flash_op = (struct ipc_msg_flash_op_t *)buffer;
	length = flash_op->length;
	offset = flash_op->offset;

	switch(flash_op->opcode) {
		case IPC_MSG_FLASH_ERASE:
			OTA_LOG("IPC_MSG_FLASH_ERASE: offset = %08x, length = %04x\r\n", offset, length);
			offset &= 0xfffff000;
			while(length) {
				qspi_flash_erase(offset);
				if(length > 0x1000) {
					length -= 0x1000;
					offset += 0x1000;
				}
				else {
					length = 0;
				}
			}

			// send response to MCU
			local_flash_op.opcode = IPC_MSG_FLASH_ERASE_ACK;
			local_flash_op.status = IPC_MSG_FLASH_OK;
			local_flash_op.length = flash_op->length;
			local_flash_op.offset = flash_op->offset;
			ipc_msg_with_payload_send(IPC_MSG_FLASH_OPERATION, (void *)&local_flash_op,
										sizeof(struct ipc_msg_flash_op_t),
										NULL, 0,
										NULL);
			break;
		case IPC_MSG_FLASH_READ:
			OTA_LOG("IPC_MSG_FLASH_READ: offset = %08x, length = %04x\r\n", offset, length);
			// send response to MCU
			local_flash_op.opcode = IPC_MSG_FLASH_READ_ACK;
			local_flash_op.status = IPC_MSG_FLASH_OK;
			local_flash_op.length = flash_op->length;
			local_flash_op.offset = flash_op->offset;
			ipc_msg_with_payload_send(IPC_MSG_FLASH_OPERATION, (void *)&local_flash_op,
										sizeof(struct ipc_msg_flash_op_t),
										(void *)(flash_op->offset | QSPI1_DAC_ADDRESS), local_flash_op.length,
										NULL);
			break;
		case IPC_MSG_FLASH_WRITE:
			OTA_LOG("IPC_MSG_FLASH_WRITE: offset = %08x, length = %04x\r\n", offset, length);

			qspi_flash_write(flash_op->offset, flash_op->length, buffer+sizeof(struct ipc_msg_flash_op_t));

			// send response to MCU
			local_flash_op.opcode = IPC_MSG_FLASH_WRITE_ACK;
			local_flash_op.status = IPC_MSG_FLASH_OK;
			local_flash_op.length = flash_op->length;
			local_flash_op.offset = flash_op->offset;
			ipc_msg_with_payload_send(IPC_MSG_FLASH_OPERATION, (void *)&local_flash_op,
										sizeof(struct ipc_msg_flash_op_t),
										NULL, 0,
										NULL);
			break;
		case IPC_MSG_FLASH_COPY:
			ota_flash_copy(flash_op->offset, flash_op->p.dest_offset, flash_op->length);
			break;
		case IPC_MSG_FLASH_CHECK_CRC:
			{
				uint32_t crc_length, crc_expected, crc_result;
				crc_expected = flash_op->p.checksum;

				crc_length = flash_op->length;
				crc_result = crc32(0xffffffff, (void *)(flash_op->offset | QSPI1_DAC_ADDRESS), crc_length);

				OTA_LOG("IPC_MSG_FLASH_CHECK_CRC: offset = %08x, length = %08x, crc_result = %08x, crc_expected = %08x\r\n", offset, crc_length, crc_result, crc_expected);
				// send response to MCU
				local_flash_op.opcode = IPC_MSG_FLASH_CHECK_CRC_ACK;
				if(crc_result == crc_expected) {
					local_flash_op.status = IPC_MSG_FLASH_OK;
				}
				else {
					local_flash_op.status = IPC_MSG_FLASH_ERROR;
				}
				local_flash_op.length = flash_op->length;
				local_flash_op.offset = flash_op->offset;
				ipc_msg_with_payload_send(IPC_MSG_FLASH_OPERATION, (void *)&local_flash_op,
											sizeof(struct ipc_msg_flash_op_t),
											NULL, 0,
											NULL);
			}
			break;
	}
}


