/*
 * flash.c
 *
 *  Created on: 2018-1-25
 *      Author: owen
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

//#include "sys_utils.h"

#include "plf.h"
#include "qspi.h"
//#include "driver_flash.h"

#define FLASH_ID_PUYA_P25Q40H       0x00136085
#define FLASH_ID_PUYA_P25Q20H       0x00126085

#define FLASH_SEC_REG_ERASE_OPCODE      0x44
#define FLASH_SEC_REG_PROGRAM_OPCODE    0x42
#define FLASH_SEC_REG_READ_OPCODE      	0x48

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

extern const struct qspi_stig_reg_t sector_erase_cmd;
extern const struct qspi_stig_reg_t read_id_cmd;
extern const struct qspi_stig_reg_t read_status_cmd;
extern const struct qspi_stig_reg_t read_status_h_cmd;
extern const struct qspi_stig_reg_t write_enable_cmd;
extern const struct qspi_stig_reg_t write_volatile_enable_cmd;
extern const struct qspi_stig_reg_t write_status_cmd;
extern const struct qspi_stig_reg_t block_erase_cmd;
extern const struct qspi_stig_reg_t write_cmd;
extern const struct qspi_stig_reg_t write_disable_cmd;
extern const struct qspi_stig_reg_t read_cmd;
extern const struct qspi_stig_reg_t deep_sleep_cmd;
extern const struct qspi_stig_reg_t wakeup_cmd;

//For securuty_sec_erase
const struct qspi_stig_reg_t sec_sector_erase_cmd = {
    .enable_bank = 0,
    .dummy_cycles = 0,
    .write_bytes = 0,
    .enable_write = 0,
    .addr_bytes = QSPI_STIG_ADDR_BYTES_3,
    .enable_mode = 0,
    .enable_cmd_addr = 1,
    .read_bytes = 0,
    .enable_read = 0,
    .opcode = FLASH_SEC_REG_ERASE_OPCODE,
};
//For securuty_sec_read
const struct qspi_stig_reg_t sec_sector_read_cmd = {
    .enable_bank = 0,
    .dummy_cycles = 8,
    .write_bytes = 0,
    .enable_write = 0,
    .addr_bytes = QSPI_STIG_ADDR_BYTES_3,
    .enable_mode = 0,
    .enable_cmd_addr = 1,
    .read_bytes = 0,
    .enable_read = 1,
    .opcode = FLASH_SEC_REG_READ_OPCODE,
};
//For securuty_sec_write
const struct qspi_stig_reg_t sec_sector_write_cmd = {
    .enable_bank = 0,
    .dummy_cycles = 0,
    .write_bytes = 0,
    .enable_write = 1,
    .addr_bytes = QSPI_STIG_ADDR_BYTES_3,
    .enable_mode = 0,
    .enable_cmd_addr = 1,
    .read_bytes = 0,
    .enable_read = 0,
    .opcode = FLASH_SEC_REG_PROGRAM_OPCODE,
};

/* used to access flash */
const struct qspi_stig_reg_t page_erase_cmd = {
    .enable_bank = 0,
    .dummy_cycles = 0,
    .write_bytes = 0,
    .enable_write = 0,
    .addr_bytes = 2,    // QSPI_STIG_ADDR_BYTES_3,
    .enable_mode = 0,
    .enable_cmd_addr = 1,
    .read_bytes = 0,
    .enable_read = 0,
    .opcode = 0x81,
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

void enable_cache(uint8_t invalid_ram);
void disable_cache(void);
uint16_t flash_read_status(void);
uint8_t qspi_flash_erase(uint32_t offset, uint32_t size);

__attribute__((section("iram_section"))) uint16_t flash_read_status(void)
{
    uint8_t *status;
    uint16_t status_entity;

    status = (uint8_t *)&status_entity;

    qspi_stig_cmd(read_status_cmd, QSPI_STIG_CMD_READ, 1, &status[0]);
    qspi_stig_cmd(read_status_h_cmd, QSPI_STIG_CMD_READ, 1, &status[1]);

    return status_entity;
}


__attribute__((section("iram_section")))void flash_wait_wip_clear(void)
{
    uint8_t status;
    
    while(1) {
        co_delay_100us(1);
        qspi_stig_cmd(read_status_cmd, QSPI_STIG_CMD_READ, 1, &status);
        if((status & 0x01) == 0) {
            break;
        }
    }
}

__attribute__((section("iram_section"))) uint16_t flash_read_status_imp(bool read_high)
{
    uint8_t *status;
    uint16_t status_entity;

    status = (uint8_t *)&status_entity;

    qspi_stig_cmd(read_status_cmd, QSPI_STIG_CMD_READ, 1, &status[0]);
    if(read_high) {
        qspi_stig_cmd(read_status_h_cmd, QSPI_STIG_CMD_READ, 1, &status[1]);
    }

    return status_entity;
}

#if 1
__attribute__((section("iram_section"))) void flash_write_status_imp(uint16_t status, bool write_high)
{
    uint8_t count = 1;

    if(write_high) {
        count++;
    }
    qspi_stig_cmd(write_volatile_enable_cmd, QSPI_STIG_CMD_EXE, 0, 0);
    qspi_stig_cmd(write_status_cmd, QSPI_STIG_CMD_WRITE, count, (void *)&status);

    flash_wait_wip_clear();
}
#endif


__attribute__((section("iram_section"))) void flash_protect_enable(void)
{
    uint8_t status = flash_read_status_imp(false);
    status = (status & 0x83) | (0x1F << 2);
    flash_write_status_imp(status,false);
}

__attribute__((section("iram_section"))) void flash_protect_disable(void)
{
    uint8_t status = flash_read_status_imp(false);
    //printf("sta=%x\r\n",status);
    status = (status & 0x83);
    flash_write_status_imp(status,false);
}

__attribute__((section("iram_section"))) void flash_protect_enable_nonvolatile(void)
{
    uint8_t status = flash_read_status_imp(false);
    if((status & 0x7c) == 0){
        status = status | (0x1F << 2);
        qspi_stig_cmd(write_enable_cmd, QSPI_STIG_CMD_EXE, 0, 0);
        qspi_stig_cmd(write_status_cmd, QSPI_STIG_CMD_WRITE, 1, (void *)&status);
        flash_wait_wip_clear();
    }
}


