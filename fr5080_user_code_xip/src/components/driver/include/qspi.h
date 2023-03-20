/*
 * qspi.h
 *
 *  Created on: 2018-1-15
 *      Author: owen
 */

#ifndef _QSPI_H
#define _QSPI_H

#include <stdint.h>

#define INLINE          static __attribute__((always_inline))

enum qspi_baud_rate_t {
    QSPI_BAUDRATE_DIV_2,
    QSPI_BAUDRATE_DIV_4,
    QSPI_BAUDRATE_DIV_6,
    QSPI_BAUDRATE_DIV_8,
    QSPI_BAUDRATE_DIV_10,
    QSPI_BAUDRATE_DIV_12,
    QSPI_BAUDRATE_DIV_14,
    QSPI_BAUDRATE_DIV_16,
    QSPI_BAUDRATE_DIV_18,
    QSPI_BAUDRATE_DIV_20,
    QSPI_BAUDRATE_DIV_22,
    QSPI_BAUDRATE_DIV_24,
    QSPI_BAUDRATE_DIV_26,
    QSPI_BAUDRATE_DIV_28,
    QSPI_BAUDRATE_DIV_30,
    QSPI_BAUDRATE_DIV_32,
};

struct qspi_config_reg_t {  //@0x00
    uint32_t enable:1;
    uint32_t cpol:1;
    uint32_t cpha:1;
    uint32_t reserved0:1;
    uint32_t hold_pin:1;
    uint32_t reset_pin:1;
    uint32_t reset_pin_conf:1;
    uint32_t enable_DAC:1;  //direct access controller
    uint32_t enable_legacy:1;
    uint32_t peri_sel:1;
    uint32_t peri_sel_line:4;   //?peri_sel????
    uint32_t write_en_pin:1;
    uint32_t enable_DMA:1;
    uint32_t enable_AHB_remap:1;
    uint32_t enable_XIP_next_R:1;   //enter XIP Mode on next READ
    uint32_t enalbe_XIP:1;  //enter XIP Mode immediately
    uint32_t baud_rate:4;
    uint32_t enable_AHB_decoder:1;
    uint32_t enable_DRT_prot:1;
    uint32_t reserved1:6;
    uint32_t status:1;
};

enum qspi_wire_type_t {
    QSPI_WIRE_TYPE_STAND,
    QSPI_WIRE_TYPE_DIO,
    QSPI_WIRE_TYPE_QIO,
};

struct qspi_read_ins_reg_t {        //@0x04
    uint32_t opcode_no_XIP:8;
    uint32_t instruction_type:2;
    uint32_t enable_DDR:1;
    uint32_t reserved0:1;
    uint32_t addr_type:2;
    uint32_t reserved1:2;
    uint32_t data_type:2;
    uint32_t reserved2:2;
    uint32_t enable_mode:1;
    uint32_t reserved3:3;
    uint32_t dummy_cycles:5;
    uint32_t reserved4:3;
};

struct qspi_write_ins_reg_t {       //@0x08
    uint32_t opcode:8;
    uint32_t disable_WEL:1;
    uint32_t reserved0:3;
    uint32_t addr_type:2;
    uint32_t reserved1:2;
    uint32_t data_type:2;
    uint32_t reserved2:6;
    uint32_t dummy_cycles:5;
    uint32_t reserved3:3;
};

struct qspi_device_delay_reg_t {    //@0x0c
    uint32_t sel_start_offset:8;
    uint32_t sel_end_offset:8;
    uint32_t sel_dessert_diff:8;
    uint32_t sel_dessert:8;
};

struct qspi_read_cap_reg_t {        //@0x10
    uint32_t enable_loopback_clk:1;
    uint32_t delay_capture:4;
    uint32_t sample_edge:1;
    uint32_t reserved0:10;
    uint32_t delay_trans:4;
    uint32_t reserved1:12;
};

enum qspi_flash_size_t {
    QSPI_FLASH_SIZE_512Mb,
    QSPI_FLASH_SIZE_1Gb,
    QSPI_FLASH_SIZE_2Gb,
    QSPI_FLASH_SIZE_4Gb,
};

struct qspi_device_size_cfg_t {     //@0x14
    uint32_t addr_bytes:4;  //number of address bytes. 0=1byte
    uint32_t page_bytes:12; //number of bytes per device page
    uint32_t block_bytes:5; //number of bytes per block. bytes = 2^block_bytes
    uint32_t CS0_size:2;
    uint32_t CS1_size:2;
    uint32_t CS2_size:2;
    uint32_t CS3_size:2;
    uint32_t reserved0:3;
};

struct qspi_dma_peri_cfg_t {        //@0x20
    uint32_t bytes_per_req:4;   //2^bytes_per_req
    uint32_t reserved0:4;
    uint32_t bytes_per_burst:4; //2^bytes_per_burst
    uint32_t reserved1:20;
};

struct qspi_sram_level_reg_t {      //@0x2c
    uint32_t read_part:16;
    uint32_t write_part:16;
};

struct qspi_write_comp_ctrl_reg_t { //@0x38
    uint32_t opcode_poll:8;
    uint32_t poll_bit_index:3;  //the bit index that should be polled
    uint32_t reserved0:2;
    uint32_t poll_polarity:1;
    uint32_t disable_poll:1;
    uint32_t enable_expiration:1;
    uint32_t poll_count:8;
    uint32_t poll_repetition_delay:8;
};

#define QSPI_INT_MODE_FAIL          0x00000001
#define QSPI_INT_UNDERFLOW          0x00000002
#define QSPI_INT_COMP_INDIRECT      0x00000004
#define QSPI_INT_FAIL_INDIRECT      0x00000008
#define QSPI_INT_FAIL_WR_PROTECT    0x00000010
#define QSPI_INT_ILLEGAL_AHB        0x00000020
#define QSPI_INT_INDIRECT_WATER     0x00000040
#define QSPI_INT_OVERFLOW           0x00000080
#define QSPI_INT_TXFF_NOFULL        0x00000100
#define QSPI_INT_TXFF_FULL          0x00000200
#define QSPI_INT_RXFF_NOEMPTY       0x00000400
#define QSPI_INT_RXFF_FULL          0x00000800
#define QSPI_INT_RD_PART_FULL       0x00001000
#define QSPI_INT_POLL_EXPIRED       0x00002000
#define QSPI_INT_COMP_STIG          0x00004000

struct qspi_write_protect_reg_t {
    uint32_t inversion:1;
    uint32_t enable:1;
    uint32_t reserved0:30;
};

struct qspi_ind_read_crtl_reg_t {   //@0x60
    uint32_t start:1;
    uint32_t cancel:1;
    uint32_t progress_status:1;
    uint32_t sram_status:1;
    uint32_t queue_status:1;
    uint32_t comp_status:1;
    uint32_t num_operation_comp:2;
    uint32_t reserved0:24;
};

struct qspi_ind_write_crtl_reg_t {   //@0x70
    uint32_t start:1;
    uint32_t cancel:1;
    uint32_t progress_status:1;
    uint32_t reserved0:1;
    uint32_t queue_status:1;
    uint32_t comp_status:1;
    uint32_t num_operation_comp:2;
    uint32_t reserved1:24;
};

enum qspi_mem_bank_bytes_t {
    QSPI_MEM_BANK_BYTES_16,
    QSPI_MEM_BANK_BYTES_32,
    QSPI_MEM_BANK_BYTES_64,
    QSPI_MEM_BANK_BYTES_128,
    QSPI_MEM_BANK_BYTES_256,
    QSPI_MEM_BANK_BYTES_512,
};

struct qspi_stig_mem_reg_t {
    uint32_t mem_bank_req:1;
    uint32_t mem_bank_status:1;
    uint32_t reserved0:6;
    uint32_t mem_bank_data:8;
    uint32_t mem_bank_req_bytes:3;
    uint32_t reserved1:1;
    uint32_t mem_bank_addr:9;
    uint32_t reserved2:3;
};

struct qspi_stig_reg_t {
    uint32_t execute:1;
    uint32_t progress_status:1;
    uint32_t enable_bank:1;
    uint32_t reserved0:4;
    uint32_t dummy_cycles:5;
    uint32_t write_bytes:3;     //0~1bytes
    uint32_t enable_write:1;
    uint32_t addr_bytes:2;      //0~1bytes
    uint32_t enable_mode:1;
    uint32_t enable_cmd_addr:1;
    uint32_t read_bytes:3;
    uint32_t enable_read:1;
    uint32_t opcode:8;
};

struct qspi_poll_flash_status_t {
    uint32_t status:8;
    uint32_t valid:1;
    uint32_t reserved0:7;
    uint32_t dummy_cycles:4;
    uint32_t reserved1:12;
};

struct qspi_regs_t {
    volatile struct qspi_config_reg_t config;                   //@0x00
    volatile struct qspi_read_ins_reg_t read_conf;
    volatile struct qspi_write_ins_reg_t write_conf;
    volatile struct qspi_device_delay_reg_t delay;
    volatile struct qspi_read_cap_reg_t read_cap;               //@0x10
    volatile struct qspi_device_size_cfg_t size_cfg;
    volatile uint32_t sram_part_cfg;
    volatile uint32_t AHB_trigger_address;
    volatile struct qspi_dma_peri_cfg_t dma_cfg;                //@0x20
    volatile uint32_t remap_address;
    volatile uint32_t mode_bits;
    volatile struct qspi_sram_level_reg_t sram_level;
    volatile uint32_t tx_threshold;                             //@0x30
    volatile uint32_t rx_threshold;
    volatile struct qspi_write_comp_ctrl_reg_t poll_cfg;
    volatile uint32_t poll_expiration;
    volatile uint32_t int_status;                               //@0x40
    volatile uint32_t int_mask;
    uint32_t reserved0[2];
    volatile uint32_t lower_write_protect;                      //@0x50
    volatile uint32_t upper_write_protect;
    volatile struct qspi_write_protect_reg_t write_protect;
    uint32_t reserved1;
    volatile struct qspi_ind_read_crtl_reg_t ind_read_ctrl;     //@0x60
    volatile uint32_t ind_read_watermark;
    volatile uint32_t ind_read_start_addr;
    volatile uint32_t ind_read_bytes;
    volatile struct qspi_ind_write_crtl_reg_t ind_write_ctrl;   //@0x70
    volatile uint32_t ind_write_watermark;
    volatile uint32_t ind_write_start_addr;
    volatile uint32_t ind_write_bytes;
    volatile uint32_t ind_range_width;                          //@0x80
    uint32_t reserved2[2];
    volatile struct qspi_stig_mem_reg_t cmd_ctrl_mem;
    volatile struct qspi_stig_reg_t cmd_ctrl;         //@0x90
    volatile uint32_t cmd_address;
    uint32_t reserved3[2];
    volatile uint32_t read_data_L;                              //@0xa0
    volatile uint32_t read_data_H;
    volatile uint32_t write_data_L;
    volatile uint32_t write_data_H;
    volatile struct qspi_poll_flash_status_t poll_status;       //@0xb0
    uint32_t reserved4[18];
    volatile uint32_t module_id;                                //@0xfc
};

enum qspi_stig_cmd_type_t {
    QSPI_STIG_CMD_READ,
    QSPI_STIG_CMD_BANK_READ,
    QSPI_STIG_CMD_WRITE,
    QSPI_STIG_CMD_EXE,
};

enum qspi_stig_addr_bytes_t {
    QSPI_STIG_ADDR_BYTES_1,
    QSPI_STIG_ADDR_BYTES_2,
    QSPI_STIG_ADDR_BYTES_3,
};

extern volatile struct qspi_regs_t *qspi_ctrl;

INLINE void qspi_cfg_set_enable(uint8_t en)
{
    qspi_ctrl->config.enable = en;
}

INLINE void qspi_cfg_set_cpol(uint8_t high)
{
    qspi_ctrl->config.cpol = high;
}

INLINE void qspi_cfg_set_cpha(uint8_t rising)
{
    qspi_ctrl->config.cpha = rising;
}

INLINE void qspi_cfg_set_hold(uint8_t hold)
{
    qspi_ctrl->config.hold_pin = hold;
}

INLINE void qspi_cfg_set_reset(uint8_t reset)
{
    qspi_ctrl->config.reset_pin = reset;
}

INLINE void qspi_cfg_set_reset_sel(uint8_t dedicated)
{
    qspi_ctrl->config.reset_pin_conf = dedicated;
}

INLINE void qspi_cfg_set_enable_dac(uint8_t en)
{
    qspi_ctrl->config.enable_DAC = en;
}

INLINE void qspi_cfg_set_enable_legacy(uint8_t en)
{
    qspi_ctrl->config.enable_legacy = en;
}

INLINE void qspi_cfg_set_write_protect(uint8_t wp)
{
    qspi_ctrl->config.write_en_pin = wp;
}

INLINE void qspi_cfg_set_enable_remap(uint8_t en)
{
    qspi_ctrl->config.enable_AHB_remap = en;
}

void qspi_cfg_set_baudrate(uint8_t baudrate);

INLINE void qspi_cfg_set_enable_AHB_decoder(uint8_t en)
{
    qspi_ctrl->config.enable_AHB_decoder = en;
}

INLINE int qspi_is_busy(void)
{
    return (qspi_ctrl->config.status == 0);
}

INLINE void qspi_read_set_opcode(uint8_t opcode)
{
    qspi_ctrl->read_conf.opcode_no_XIP = opcode;
}

INLINE void qspi_read_set_instruction_type(uint8_t type)
{
    qspi_ctrl->read_conf.instruction_type = type;
}

INLINE void qspi_read_set_address_type(uint8_t type)
{
    qspi_ctrl->read_conf.addr_type = type;
}

INLINE void qspi_read_set_data_type(uint8_t type)
{
    qspi_ctrl->read_conf.data_type = type;
}

INLINE void qspi_read_set_mode_en(uint8_t en)
{
    qspi_ctrl->read_conf.enable_mode = en;
}

INLINE void qspi_read_set_dummy_cycles(uint8_t cycles)
{
    qspi_ctrl->read_conf.dummy_cycles = cycles;
}

INLINE void qspi_write_set_opcode(uint8_t opcode)
{
    qspi_ctrl->write_conf.opcode = opcode;
}

INLINE void qspi_write_set_wel_dis(uint8_t disable)
{
    qspi_ctrl->write_conf.disable_WEL = disable;
}

INLINE void qspi_write_set_address_type(uint8_t type)
{
    qspi_ctrl->write_conf.addr_type = type;
}

INLINE void qspi_write_set_data_type(uint8_t type)
{
    qspi_ctrl->write_conf.data_type = type;
}

INLINE void qspi_write_set_dummy_cycles(uint8_t cycles)
{
    qspi_ctrl->write_conf.dummy_cycles = cycles;
}

INLINE void qspi_set_remap_address(uint32_t address)
{
    qspi_ctrl->remap_address = address;
}

INLINE void qspi_set_mode_bit(uint8_t mode)
{
    qspi_ctrl->mode_bits = (uint32_t)mode;
}

INLINE void qspi_poll_set_opcode(uint8_t opcode)
{
    qspi_ctrl->poll_cfg.opcode_poll = opcode;
}

INLINE void qspi_poll_set_bit_index(uint8_t index)
{
    qspi_ctrl->poll_cfg.poll_bit_index = index;
}

INLINE void qspi_poll_set_polarity(uint8_t pol)
{
    qspi_ctrl->poll_cfg.poll_polarity = pol;
}

INLINE void qspi_poll_set_disable(uint8_t dis)
{
    qspi_ctrl->poll_cfg.disable_poll = dis;
}

INLINE void qspi_poll_set_expire(uint8_t en, uint32_t duration)
{
    qspi_ctrl->poll_cfg.enable_expiration = en;
    if(en) {
        qspi_ctrl->poll_expiration = duration;
    }
}

INLINE void qspi_poll_set_poll_count(uint8_t count)
{
    qspi_ctrl->poll_cfg.poll_count = count;
}

INLINE void qspi_poll_set_poll_delay(uint8_t delay)
{
    qspi_ctrl->poll_cfg.poll_repetition_delay = delay;
}

INLINE void qspi_set_cmd_addr(uint32_t addr)
{
    qspi_ctrl->cmd_address = addr;
}

INLINE void qspi_stig_set_opcode(uint8_t opcode)
{
    qspi_ctrl->cmd_ctrl.opcode = opcode;
}

INLINE void qspi_stig_set_read_en(uint8_t en)
{
    qspi_ctrl->cmd_ctrl.enable_read = en;
}

INLINE void qspi_stig_set_read_bytes(uint8_t bytes)
{
    qspi_ctrl->cmd_ctrl.read_bytes = bytes;
}

INLINE void qspi_stig_set_addr(uint8_t en, uint8_t bytes, uint32_t addr)
{
    qspi_ctrl->cmd_ctrl.enable_cmd_addr = en;
    qspi_ctrl->cmd_ctrl.addr_bytes = bytes;
    if(en) {
        qspi_ctrl->cmd_address = addr;
    }
}

INLINE void qspi_stig_set_mode_en(uint8_t en)
{
    qspi_ctrl->cmd_ctrl.enable_mode = en;
}

INLINE void qspi_stig_set_write_en(uint8_t en)
{
    qspi_ctrl->cmd_ctrl.enable_write = en;
}

INLINE void qspi_stig_set_write_bytes(uint8_t bytes)
{
    qspi_ctrl->cmd_ctrl.write_bytes = bytes;
}

INLINE void qspi_stig_set_dummy_cycles(uint8_t cycles)
{
    qspi_ctrl->cmd_ctrl.dummy_cycles = cycles;
}

INLINE void qspi_stig_set_mem_bank(uint8_t en, uint8_t bytes)
{
    qspi_ctrl->cmd_ctrl.enable_bank = en;
    if(en) {
        qspi_ctrl->cmd_ctrl_mem.mem_bank_req_bytes = bytes;
    }
}

INLINE void qspi_set_delay_start_offset(uint8_t offset)
{
	qspi_ctrl->delay.sel_start_offset = offset;
}

INLINE void qspi_set_delay_end_offset(uint8_t offset)
{
	qspi_ctrl->delay.sel_end_offset = offset;
}

INLINE void qspi_set_delay_capture(uint8_t capture)
{
    qspi_ctrl->read_cap.delay_capture = capture;
}

INLINE void qspi_cs_control(uint8_t en)
{
    if(en)
        qspi_ctrl->config.peri_sel_line = 0x0;	//重新将CS线控制交给硬件
    else
        qspi_ctrl->config.peri_sel_line = 0xf;  //取消CS线的硬件控制
}

INLINE void qspi_set_address_length(uint8_t length)
{
    qspi_ctrl->size_cfg.addr_bytes = length;
}

int qspi_stig_cmd(struct qspi_stig_reg_t cmd, enum qspi_stig_cmd_type_t type, int len, uint8_t *buffer);

#endif /* _QSPI_H */

