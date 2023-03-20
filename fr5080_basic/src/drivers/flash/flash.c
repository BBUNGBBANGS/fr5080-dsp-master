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

void qspi_flash_init_controller(uint8_t type, uint8_t fast_read)
{
    while(qspi_is_busy());

    if(type == 4) {
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

    if(type == 2) {
        // Dual Output Read: BBH A23-A16 A15-A8 A7-A0 M7-M0 (D7-D0) (3) (continuous)
        //printf("two line read\r\n");
    	if(fast_read){
    		qspi_read_set_opcode(FLASH_READ_DUAL_OPCODE_2);//0xBB
            qspi_read_set_instruction_type(QSPI_WIRE_TYPE_STAND);
            qspi_read_set_address_type(QSPI_WIRE_TYPE_DIO);
            qspi_read_set_data_type(QSPI_WIRE_TYPE_DIO);
            qspi_read_set_dummy_cycles(0);
            qspi_read_set_mode_en(1);
    	}else{
    		qspi_read_set_opcode(FLASH_READ_DUAL_OPCODE_1);//0x3B
            qspi_read_set_instruction_type(QSPI_WIRE_TYPE_STAND);
            qspi_read_set_address_type(QSPI_WIRE_TYPE_STAND);
            qspi_read_set_data_type(QSPI_WIRE_TYPE_STAND);
            qspi_read_set_dummy_cycles(8);
            qspi_read_set_mode_en(0);
    	}
        qspi_set_mode_bit(0x0);
    }

    if(type == 1) {
        // single Output Fast Read: 03H A23-A16 A15-A8 A7-A0 (D7-D0) (3) (continuous)    
        qspi_read_set_opcode(FLASH_READ_OPCODE);    
        qspi_read_set_instruction_type(QSPI_WIRE_TYPE_STAND);    
        qspi_read_set_address_type(QSPI_WIRE_TYPE_STAND);    
        qspi_read_set_data_type(QSPI_WIRE_TYPE_STAND);    
        qspi_read_set_dummy_cycles(0);    
        qspi_read_set_mode_en(0);    
        qspi_set_mode_bit(0);
    }

#if 1
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
#endif

    //init configuration register
    qspi_cfg_set_cpol(1);
    qspi_cfg_set_cpha(1);
    qspi_cfg_set_enable_dac(1);
    qspi_cfg_set_enable_legacy(0);
    qspi_cfg_set_enable_remap(1);
    qspi_cfg_set_baudrate(QSPI_BAUDRATE_DIV_16);
    //qspi_cfg_set_enable_AHB_decoder(1);

    qspi_ctrl->delay.sel_start_offset = 2;
    qspi_ctrl->delay.sel_end_offset = 2;

    qspi_set_remap_address(QSPI1_DAC_ADDRESS);

    qspi_cfg_set_enable(1);
}

uint32_t qspi_flash_init(uint8_t type,uint8_t fast_read)
{
    uint32_t flash_id;

    qspi_flash_init_controller(type,fast_read);

    flash_exit_deep_sleep();
    
    qspi_stig_cmd(read_id_cmd, QSPI_STIG_CMD_READ, 3, (uint8_t *)&flash_id);

    return (flash_id&0xffffff);
}

uint16_t flash_read_status(void)
{
    uint8_t *status;
    uint16_t status_entity;

    status = (uint8_t *)&status_entity;

    qspi_stig_cmd(read_status_cmd, QSPI_STIG_CMD_READ, 1, &status[0]);
    qspi_stig_cmd(read_status_h_cmd, QSPI_STIG_CMD_READ, 1, &status[1]);

    return status_entity;
}

void flash_write_status(uint16_t status_entity)
{
    uint8_t *status;
    uint8_t status_L;

    status = (uint8_t *)&status_entity;
    
    qspi_stig_cmd(write_enable_cmd, QSPI_STIG_CMD_EXE, 0, 0);
    qspi_stig_cmd(write_status_cmd, QSPI_STIG_CMD_WRITE, 2, &status[0]);
    qspi_stig_cmd(write_disable_cmd, QSPI_STIG_CMD_EXE, 0, 0);

    while(1) {
        co_delay_10us(5);
        qspi_stig_cmd(read_status_cmd, QSPI_STIG_CMD_READ, 1, &status_L);
        if((status[0] & 0x03) == 0) {
            break;
        }
    }
}

uint8_t flash_write(uint32_t offset, uint32_t length, uint8_t *buffer)
{
    uint32_t page_offset, page_left, tail_len;
    uint8_t status[1];
    uint32_t *dest, *src, i;

#if 1
    memcpy((void *)(offset | QSPI1_DAC_ADDRESS), buffer, length);
#else
    while((offset & 0x03) && (length != 0)) {
        *(uint8_t *)(offset+qspi_addr) = *buffer++;
        length--;
        offset++;
    }

    tail_len = length & 0x03;
    length -= tail_len;
    
    page_offset = offset & 0xFF;
    page_left = 256 - page_offset;
    if(length < page_left) {
        page_left = length;
    }
    dest = (uint32_t *)(offset+qspi_addr);
    src = (uint32_t *)buffer;
    for(i=0; i<page_left;) {
        *dest++ = *src++;
        i += 4;
    }
    length -= page_left;
    offset += page_left;
    buffer += page_left;
    
    while(length) {
        if(length > 256) {
            page_left = 256;
        }
        else {
            page_left = length;
        }
        dest = (uint32_t *)(offset+qspi_addr);
        src = (uint32_t *)buffer;
        for(i=0; i<page_left;) {
            *dest++ = *src++;
            i += 4;
        }
        length -= page_left;
        offset += page_left;
        buffer += page_left;
    }

    while(tail_len != 0) {
        *(uint8_t *)(offset+qspi_addr) = *buffer++;
        tail_len--;
        offset++;
    }

    while(1) {
        co_delay_10us(5);
        qspi_stig_cmd(read_status_cmd, QSPI_STIG_CMD_READ, 1, &status[0]);
        if((status[0] & 0x03) == 0) {
            break;
        }
    }
#endif
    return 0;
}

uint8_t flash_read(uint32_t offset, uint32_t length, uint8_t *buffer)
{
#if 1
    memcpy(buffer, (void *)(offset | QSPI1_DAC_ADDRESS), length);
#else
    uint32_t tail_len;
    uint32_t data_tmp;
    uint32_t *buffer_align;
    
    while((offset & 0x03) && (length != 0)) {
        *buffer++ = *(uint8_t *)(offset+qspi_addr);
        length--;
        offset++;
    }

    tail_len = length & 0x03;
    length -= tail_len;

    if((uint32_t)buffer & 0x03) {
        for(; length>0;) {
            data_tmp = *(uint32_t *)(offset+qspi_addr);
            memcpy(buffer, &data_tmp, 4);
            buffer += 4;
            offset += 4;
            length -= 4;
        }
    }
    else {
        buffer_align = (uint32_t *)buffer;
        for(; length>0;) {
            *buffer_align++ = *(uint32_t *)(offset+qspi_addr);
            offset += 4;
            length -= 4;
        }
        buffer = (uint8_t *)buffer_align;
    }

    while(tail_len != 0) {
        *buffer++ = *(uint8_t *)(offset+qspi_addr);
        tail_len--;
        offset++;
    }
#endif
    
    return 0;
}

uint8_t flash_erase(uint32_t offset, uint32_t size)
{
    uint8_t status;

    if(size == 0) {
        size = 0x1000;
    }
    offset &= 0xFFFFF000;
    while(size) {
        qspi_stig_cmd(write_enable_cmd, QSPI_STIG_CMD_EXE, 0, 0);
        qspi_set_cmd_addr(offset);
        qspi_stig_cmd(sector_erase_cmd, QSPI_STIG_CMD_EXE, 0, 0);
        offset += 0x1000;
        if(size > 0x1000)
            size -= 0x1000;
        else {
            size = 0;
        }
        while(1) {
            co_delay_10us(5);
            qspi_stig_cmd(read_status_cmd, QSPI_STIG_CMD_READ, 1, &status);
            if((status & 0x01) == 0) {
                break;
            }
        }
    }
    return 0;
}

void flash_enter_deep_sleep(void)
{
    qspi_stig_cmd(deep_sleep_cmd, QSPI_STIG_CMD_EXE, 0, 0);
    co_delay_10us(2);
}

void flash_exit_deep_sleep(void)
{
    qspi_stig_cmd(wakeup_cmd, QSPI_STIG_CMD_EXE, 0, 0);
    co_delay_10us(2);
}


