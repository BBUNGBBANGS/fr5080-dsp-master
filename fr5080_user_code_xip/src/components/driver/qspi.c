/*
 * qspi.c
 *
 *  Created on: 2018-12-17
 *      Author: Administrator
 */

/*
 * ���ļ���Ҫ�����л�qspi��������ã��������lcd������д������flash ��д������
 */

#include <stdint.h>
#include <string.h>

#include "co_util.h"

#include "flash.h"
#include "qspi.h"
#include "plf.h"

extern const struct qspi_stig_reg_t read_status_cmd;
extern uint8_t flash_write(uint32_t offset, uint32_t length, uint8_t *buffer);
extern uint8_t flash_read(uint32_t offset, uint32_t length, uint8_t *buffer);
extern uint8_t flash_erase(uint32_t offset, uint32_t size);

__attribute__((section("iram_section"))) void qspi_flash_erase(uint32_t offset, uint32_t size)
{
	flash_protect_disable();
	GLOBAL_INT_DISABLE();
	// �ȴ�qspi controller���ڿ���״̬���ڽ��к�������
	while(qspi_is_busy());
	flash_erase(offset, size);
	while(qspi_is_busy());

	GLOBAL_INT_RESTORE();
	flash_protect_enable();
}

__attribute__((section("iram_section"))) static void qspi_flash_write_sub(uint32_t offset, uint32_t length, uint8_t *buffer)
{
	uint8_t wel_dis, status[1];
	uint32_t page_bytes;
	uint8_t write_addr_length;

	GLOBAL_INT_DISABLE();
	// �ȴ�qspi controller���ڿ���״̬���ڽ��к�������
	while(qspi_is_busy());

	flash_write(offset, length, (void *)buffer);
	while(1) {
		co_delay_10us(10);
		qspi_stig_cmd(read_status_cmd, QSPI_STIG_CMD_READ, 1, &status[0]);
		if((status[0] & 0x03) == 0) {
			break;
		}
	}

	while(qspi_is_busy());

	GLOBAL_INT_RESTORE();
}

__attribute__((section("iram_section"))) void qspi_flash_write(uint32_t offset, uint32_t length, uint8_t *buffer)
{
	uint32_t first_page_left;
	uint32_t sub_length;

	flash_protect_disable();
	first_page_left = 0x100 - (offset & 0xff);
	if(length > first_page_left) {
		sub_length = first_page_left;
		}
	else {
		sub_length = length;
	}
	qspi_flash_write_sub(offset, sub_length, buffer);
	buffer += sub_length;
	length -= sub_length;
	offset += sub_length;

	while(length) {
		sub_length = (length > 0x100) ? 0x100: length;
		qspi_flash_write_sub(offset, sub_length, buffer);
		buffer += sub_length;
		length -= sub_length;
		offset += sub_length;
	}
    flash_protect_enable();
}

__attribute__((section("iram_section"))) void qspi_flash_read(uint32_t offset, uint32_t length, uint8_t *buffer)
{
	//memcpy(buffer, (offset | QSPI1_DAC_ADDRESS), length);
	flash_read(offset, length, buffer);
}

