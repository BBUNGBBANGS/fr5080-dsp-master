/*
 * flash.c
 *
 *  Created on: 2018-1-25
 *      Author: owen
 */

#include <stdint.h>
#include <string.h>

#include "co_util.h"

#include "plf.h"
#include "qspi.h"
#include "flash.h"

const struct qspi_stig_reg_t sector_erase_cmd = {
    .enable_bank = 0,
    .dummy_cycles = 0,
    .write_bytes = 0,
    .enable_write = 0,
    .addr_bytes = QSPI_STIG_ADDR_BYTES_3,
    .enable_mode = 0,
    .enable_cmd_addr = 1,
    .read_bytes = 0,
    .enable_read = 0,
    .opcode = FLASH_SECTORE_ERASE_OPCODE,
};

const struct qspi_stig_reg_t read_id_cmd = {
    .enable_bank = 0,
    .dummy_cycles = 0,
    .write_bytes = 0,
    .enable_write = 0,
    .addr_bytes = 0,
    .enable_mode = 0,
    .enable_cmd_addr = 0,
    .read_bytes = 3,
    .enable_read = 1,
    .opcode = FLASH_READ_IDENTIFICATION,
};

const struct qspi_stig_reg_t read_status_cmd = {
    .enable_bank = 0,
    .dummy_cycles = 0,
    .write_bytes = 0,
    .enable_write = 0,
    .addr_bytes = 0,
    .enable_mode = 0,
    .enable_cmd_addr = 0,
    .read_bytes = 1,
    .enable_read = 1,
    .opcode = 0x05,
};

const struct qspi_stig_reg_t read_status_h_cmd = {
    .enable_bank = 0,
    .dummy_cycles = 0,
    .write_bytes = 0,
    .enable_write = 0,
    .addr_bytes = 0,
    .enable_mode = 0,
    .enable_cmd_addr = 0,
    .read_bytes = 1,
    .enable_read = 1,
    .opcode = 0x35,
};

const struct qspi_stig_reg_t write_enable_cmd = {
    .enable_bank = 0,
    .dummy_cycles = 0,
    .write_bytes = 0,
    .enable_write = 0,
    .addr_bytes = 0,
    .enable_mode = 0,
    .enable_cmd_addr = 0,
    .read_bytes = 0,
    .enable_read = 0,
    .opcode = 0x06,
};

const struct qspi_stig_reg_t write_volatile_enable_cmd = {
    .enable_bank = 0,
    .dummy_cycles = 0,
    .write_bytes = 0,
    .enable_write = 0,
    .addr_bytes = 0,
    .enable_mode = 0,
    .enable_cmd_addr = 0,
    .read_bytes = 0,
    .enable_read = 0,
    .opcode = 0x50,
};

const struct qspi_stig_reg_t write_status_cmd = {
    .enable_bank = 0,
    .dummy_cycles = 0,
    .write_bytes = 1,
    .enable_write = 1,
    .addr_bytes = 0,
    .enable_mode = 0,
    .enable_cmd_addr = 0,
    .read_bytes = 0,
    .enable_read = 0,
    .opcode = 0x01,
};

const struct qspi_stig_reg_t block_erase_cmd = {
    .enable_bank = 0,
    .dummy_cycles = 0,
    .write_bytes = 0,
    .enable_write = 0,
    .addr_bytes = QSPI_STIG_ADDR_BYTES_3,
    .enable_mode = 0,
    .enable_cmd_addr = 1,
    .read_bytes = 0,
    .enable_read = 0,
    .opcode = 0xD8,
};
const struct qspi_stig_reg_t write_cmd = {
    .enable_bank = 0,
    .dummy_cycles = 0,
    .write_bytes = 0,
    .enable_write = 1,
    .addr_bytes = QSPI_STIG_ADDR_BYTES_3,
    .enable_mode = 0,
    .enable_cmd_addr = 1,
    .read_bytes = 0,
    .enable_read = 0,
    .opcode = 0x02,
};
const struct qspi_stig_reg_t write_disable_cmd = {
    .enable_bank = 0,
    .dummy_cycles = 0,
    .write_bytes = 0,
    .enable_write = 0,
    .addr_bytes = 0,
    .enable_mode = 0,
    .enable_cmd_addr = 0,
    .read_bytes = 0,
    .enable_read = 0,
    .opcode = 0x04,
};
const struct qspi_stig_reg_t read_cmd = {
    .enable_bank = 0,
    .dummy_cycles = 0,
    .write_bytes = 0,
    .enable_write = 0,
    .addr_bytes = QSPI_STIG_ADDR_BYTES_3,
    .enable_mode = 0,
    .enable_cmd_addr = 1,
    .read_bytes = 0,
    .enable_read = 1,
    .opcode = 0x03,
};
const struct qspi_stig_reg_t deep_sleep_cmd = {
    .enable_bank = 0,
    .dummy_cycles = 0,
    .write_bytes = 0,
    .enable_write = 0,
    .addr_bytes = 0,
    .enable_mode = 0,
    .enable_cmd_addr = 0,
    .read_bytes = 0,
    .enable_read = 0,
    .opcode = 0xB9,
};
const struct qspi_stig_reg_t wakeup_cmd = {
    .enable_bank = 0,
    .dummy_cycles = 0,
    .write_bytes = 0,
    .enable_write = 0,
    .addr_bytes = 0,
    .enable_mode = 0,
    .enable_cmd_addr = 0,
    .read_bytes = 0,
    .enable_read = 0,
    .opcode = 0xAB,
};

void qspi_flash_enable_quad(void)
{
    uint8_t status[2];

    qspi_stig_cmd(read_status_cmd, QSPI_STIG_CMD_READ, 1, &status[0]);
    qspi_stig_cmd(read_status_h_cmd, QSPI_STIG_CMD_READ, 1, &status[1]);
    if((status[1] & 0x02) == 0x02) {
        return;
    }
    status[1] |= 0x02;  //enable quad mode
    qspi_stig_cmd(write_enable_cmd, QSPI_STIG_CMD_EXE, 0, 0);
    qspi_stig_cmd(write_status_cmd, QSPI_STIG_CMD_WRITE, 2, &status[0]);
    qspi_stig_cmd(write_disable_cmd, QSPI_STIG_CMD_EXE, 0, 0);

    while(1) {
        co_delay_10us(5);
        qspi_stig_cmd(read_status_cmd, QSPI_STIG_CMD_READ, 1, &status[0]);
        if((status[0] & 0x03) == 0) {
            break;
        }
    }
}

void qspi_flash_init_controller(uint8_t read_type, uint8_t write_type)
{
    while(qspi_is_busy());

    if(read_type == 4) {
    	//test qspi write 4line read
    	//printf("qspi 4 line read\r\n");
    	qspi_read_set_opcode(FLASH_PAGE_QUAL_READ_OPCODE);	
    	qspi_read_set_instruction_type(QSPI_WIRE_TYPE_STAND);
    	qspi_read_set_address_type(QSPI_WIRE_TYPE_QIO);
    	qspi_read_set_data_type(QSPI_WIRE_TYPE_QIO);
    	qspi_read_set_dummy_cycles(4);
    	qspi_read_set_mode_en(1);
    	qspi_set_mode_bit(0);//8 bits data after addr
    }

    if(read_type == 2) {
        // Dual Output Read: BBH A23-A16 A15-A8 A7-A0 M7-M0 (D7-D0) (3) (continuous)
        //printf("two line read\r\n");
        qspi_read_set_opcode(FLASH_READ_DUAL_OPCODE);
        qspi_read_set_instruction_type(QSPI_WIRE_TYPE_STAND);
        qspi_read_set_address_type(QSPI_WIRE_TYPE_DIO);
        qspi_read_set_data_type(QSPI_WIRE_TYPE_DIO);
        qspi_read_set_dummy_cycles(0);
        qspi_read_set_mode_en(1);
        qspi_set_mode_bit(0x0);
    }

    if(read_type == 1) {
        // single Output Fast Read: 03H A23-A16 A15-A8 A7-A0 (D7-D0) (3) (continuous)    
        qspi_read_set_opcode(FLASH_READ_OPCODE);    
        qspi_read_set_instruction_type(QSPI_WIRE_TYPE_STAND);    
        qspi_read_set_address_type(QSPI_WIRE_TYPE_STAND);    
        qspi_read_set_data_type(QSPI_WIRE_TYPE_STAND);    
        qspi_read_set_dummy_cycles(0);    
        qspi_read_set_mode_en(0);    
        qspi_set_mode_bit(0);
    }

    if(write_type == 4) {
        qspi_write_set_opcode(FLASH_PAGE_QUAL_PROGRAM_OPCODE);
        qspi_write_set_address_type(QSPI_WIRE_TYPE_STAND);
        qspi_write_set_data_type(QSPI_WIRE_TYPE_QIO);
        qspi_write_set_dummy_cycles(0);
        qspi_poll_set_opcode(0x05);
        qspi_poll_set_bit_index(0);
        qspi_poll_set_polarity(0);
        qspi_poll_set_expire(0, 0);
        qspi_poll_set_poll_count(2);
        qspi_poll_set_poll_delay(16);
    }

    if(write_type == 2) {
        // Dual Page Program: A2H A23-A16 A15-A8 A7-A0 (D7-D0) Next byte
        //printf("two line write\r\n");
        qspi_write_set_opcode(FLASH_PAGE_DUAL_PROGRAM_OPCODE);
        qspi_write_set_address_type(QSPI_WIRE_TYPE_STAND);
        qspi_write_set_data_type(QSPI_WIRE_TYPE_DIO);
        qspi_write_set_dummy_cycles(0);
        qspi_poll_set_opcode(0x05);
        qspi_poll_set_bit_index(0);
        qspi_poll_set_polarity(0);
        qspi_poll_set_expire(0, 0);
        qspi_poll_set_poll_count(2);
        qspi_poll_set_poll_delay(16);
    }

    if(write_type == 1) {
        // single Page Program: 02H A23-A16 A15-A8 A7-A0 (D7-D0) Next byte
        qspi_write_set_opcode(FLASH_PAGE_PROGRAM_OPCODE);
        qspi_write_set_address_type(QSPI_WIRE_TYPE_STAND);
        qspi_write_set_data_type(QSPI_WIRE_TYPE_STAND);
        qspi_write_set_dummy_cycles(0);
        qspi_poll_set_opcode(0x05);
        qspi_poll_set_bit_index(0);
        qspi_poll_set_polarity(0);
        qspi_poll_set_expire(0, 0);
        qspi_poll_set_poll_count(2);
        qspi_poll_set_poll_delay(16);
    }

    if((write_type == 4) || (read_type == 4)) {
        //qspi_flash_enable_quad();
    }

    //init configuration register
    qspi_cfg_set_cpol(1);
    qspi_cfg_set_cpha(1);
    qspi_cfg_set_enable_dac(1);
    qspi_cfg_set_enable_legacy(0);
    qspi_cfg_set_enable_remap(1);
    qspi_cfg_set_baudrate(QSPI_BAUDRATE_DIV_16);

    qspi_ctrl->delay.sel_start_offset = 2;
    qspi_ctrl->delay.sel_end_offset = 2;

    qspi_set_remap_address(QSPI1_DAC_ADDRESS);

    qspi_cfg_set_enable(1);
}

uint32_t qspi_flash_init(uint8_t read_type, uint8_t write_type)
{
    uint32_t flash_id;

    qspi_flash_init_controller(read_type, write_type);
    
    qspi_stig_cmd(read_id_cmd, QSPI_STIG_CMD_READ, 3, (uint8_t *)&flash_id);

    return (flash_id&0xffffff);
}
