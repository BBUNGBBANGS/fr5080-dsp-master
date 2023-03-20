/*
 * Copyright (c) 2012-2017 by Cadence Design Systems, Inc.  ALL RIGHTS RESERVED.
 * These coded instructions, statements, and computer programs are the
 * copyrighted works and confidential proprietary information of
 * Cadence Design Systems Inc.  They may be adapted and modified by bona fide
 * purchasers for internal use, but neither the original nor any adapted
 * or modified version may be disclosed or distributed to third parties
 * in any manner, medium, or form, in whole or in part, without the prior
 * written consent of Cadence Design Systems Inc.  This software and its
 * derivatives are to be executed solely on products incorporating a Cadence
 * Design Systems processor.
 */


#ifndef __XA_MSBC_ENC_API_H__
#define __XA_MSBC_ENC_API_H__

/*****************************************************************************/
/* SBC Encoder specific API definitions                                      */
/*****************************************************************************/
/* msbc_enc-specific configuration parameters */
enum xa_config_param_msbc_enc {
  XA_MSBC_ENC_CONFIG_PARAM_BITRATE      = 0
#ifdef MSBC_24KHZ
  ,XA_MSBC_ENC_CONFIG_PARAM_SAMP_FREQ     = 1
#endif /* MSBC_24KHZ */
};


/* commands */
#include "xa_apicmd_standards.h"

/* msbc_enc-specific commands */
/* (none) */

/* msbc_enc-specific command types */
/* (none) */

/* error codes */
#include "xa_error_standards.h"

#define XA_CODEC_MSBC_ENC	20

/* msbc_enc-specific error codes */
/*****************************************************************************/
/* Class 1: Configuration Errors                                             */
/*****************************************************************************/
/* Nonfatal Errors */
/* Fatal Errors */
#ifdef MSBC_24KHZ
enum xa_error_fatal_config_msbc_enc {
  XA_MSBC_ENC_CONFIG_FATAL_INVALID_SAMP_FREQ    = XA_ERROR_CODE(xa_severity_fatal, xa_class_config, XA_CODEC_MSBC_ENC, 0)
};
#endif /* MSBC_24KHZ */

/* Fatal Errors */
/* (none) */

/*****************************************************************************/
/* Class 2: Execution Errors                                                 */
/*****************************************************************************/
/* Nonfatal Errors */
enum xa_error_nonfatal_execute_msbc_enc {
  XA_MSBC_ENC_EXECUTE_NONFATAL_NEED_MORE_DATA = XA_ERROR_CODE(xa_severity_nonfatal, xa_class_execute, XA_CODEC_MSBC_ENC, 0),
};

/* Fatal Errors */
enum xa_error_fatal_execute_msbc_enc {
  XA_MSBC_ENC_EXECUTE_FATAL_OUTPUT_BUFFER_OVERFLOW = XA_ERROR_CODE(xa_severity_fatal, xa_class_execute, XA_CODEC_MSBC_ENC, 0),
};

#include "xa_type_def.h"

#if defined(__cplusplus)
extern "C" {
#endif	/* __cplusplus */
xa_codec_func_t xa_msbc_enc;
#if defined(__cplusplus)
}
#endif	/* __cplusplus */
#endif /* __XA_MSBC_ENC_API_H__ */
