/*
 * flash.h
 *
 *  Created on: 2018-1-25
 *      Author: owen
 */

#ifndef _FLASH_H
#define _FLASH_H

#include <stdint.h>

#define FLASH_READ_DEVICE_ID            0x90
#define FLASH_READ_IDENTIFICATION       0x9F
    
#define FLASH_AAI_PROGRAM_OPCODE        0xAF
#define FLASH_PAGE_PROGRAM_OPCODE       0x02
#define FLASH_READ_OPCODE               0x03
#define FLASH_FAST_READ_OPCODE          0x0B
#define FLASH_READ_DUAL_OPCODE          0xBB
#define FLASH_PAGE_DUAL_PROGRAM_OPCODE  0xA2
#define FLASH_PAGE_QUAL_READ_OPCODE     0xEB
#define FLASH_PAGE_QUAL_READ_OPCODE_2   0x6B
#define FLASH_PAGE_QUAL_PROGRAM_OPCODE  0x32

    
#define FLASH_CHIP_ERASE_OPCODE         0x60
#define FLASH_SECTORE_ERASE_OPCODE      0x20
#define FLASH_BLOCK_32K_ERASE_OPCODE    0x52
#define FLASH_BLOCK_64K_ERASE_OPCODE    0xD8
#define FLASH_ST_SECTORE_ERASE_OPCODE   0xD8
#define FLASH_ST_BULK_ERASE_OPCODE      0xC7
    
#define FLASH_WRITE_DISABLE_OPCODE      0x04
#define FLASH_WRITE_ENABLE_OPCODE       0x06
#define FLASH_WRITE_STATUS_REG_OPCODE   0x01
#define FLASH_READ_STATUS_REG_OPCODE    0x05
    
#define FLASH_ST_ID                     0x20
#define FLASH_SST_ID                    0xBF

//uint32_t flash_init(void);
uint16_t flash_read_status(void);
void flash_write_status(uint16_t status_entity);
uint8_t flash_write(uint32_t offset, uint32_t length, uint8_t *buffer);
uint8_t flash_read(uint32_t offset, uint32_t length, uint8_t *buffer);
uint8_t flash_write_legacy(uint32_t offset, uint32_t length, uint8_t *buffer);
uint8_t flash_read_legacy(uint32_t offset, uint32_t length, uint8_t *buffer);
uint8_t flash_erase(uint32_t offset, uint32_t size);
//void flash_change_bitrate(uint8_t bitrate);
void flash_enter_deep_sleep(void);
void flash_exit_deep_sleep(void);
void qspi_flash_enable_quad(void);
void qspi_flash_init_controller(uint8_t read_type, uint8_t write_type);
uint32_t qspi_flash_init(uint8_t read_type, uint8_t write_type);

#endif /* _FLASH_H */
