/*
 * qspi.c
 *
 *  Created on: 2018-1-15
 *      Author: owen
 */

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "plf.h"

#include "qspi.h"

#define QSPI_STIG_MAX_SINGLE_LEN            8
#define QSPI_STIG_BANK_DEPTH                128

volatile struct qspi_regs_t *qspi_ctrl = (volatile struct qspi_regs_t *)QSPI1_BASE;

void qspi_cfg_set_baudrate(uint8_t baudrate)
{
    qspi_ctrl->config.baud_rate = baudrate;
}

int qspi_stig_cmd(struct qspi_stig_reg_t cmd, enum qspi_stig_cmd_type_t type, int len, uint8_t *buffer)
{
    uint32_t tmp_u32[2];
    uint8_t *tmp_u8 = (uint8_t *)tmp_u32;

    if(type == QSPI_STIG_CMD_BANK_READ) {
        if(QSPI_STIG_BANK_DEPTH < len) {
            return -1;
        }
    }
    else {
        if(QSPI_STIG_MAX_SINGLE_LEN < len) {
            return -1;
        }
    }

    while(qspi_is_busy());

    if(type == QSPI_STIG_CMD_EXE) {
        qspi_ctrl->cmd_ctrl = cmd;
        qspi_ctrl->cmd_ctrl.execute = 1;
        while(qspi_ctrl->cmd_ctrl.progress_status);
    }
    else {
        if(type == QSPI_STIG_CMD_WRITE) {
            memcpy(tmp_u8, buffer, len);
            qspi_ctrl->write_data_L = tmp_u32[0];
            qspi_ctrl->write_data_H = tmp_u32[1];
            cmd.write_bytes = len - 1;
            qspi_ctrl->cmd_ctrl = cmd;
            qspi_ctrl->cmd_ctrl.execute = 1;
            while(qspi_ctrl->cmd_ctrl.progress_status);
        }
        else {
            cmd.read_bytes = len - 1;
            qspi_ctrl->cmd_ctrl = cmd;
            qspi_ctrl->cmd_ctrl.execute = 1;
            while(qspi_ctrl->cmd_ctrl.progress_status);
            if(type == QSPI_STIG_CMD_READ) {
                tmp_u32[0] = qspi_ctrl->read_data_L;
                tmp_u32[1] = qspi_ctrl->read_data_H;
                //printf("READ_L: 0x%08x, READ_H: 0x%08x.\r\n", tmp_u32[0], tmp_u32[1]);
                memcpy(buffer, tmp_u8, len);
            }
            else {
                //TBD, BANK READ
            }
        }
    }

    while(qspi_is_busy());

    return 0;
}

