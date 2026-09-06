/**
 * \file ddl_device.h
 * \brief DDL 器件级配置(系列与封装)
 *
 * 本文件由 ddl.h 强制包含(#include "ddl_device.h"),用于在编译期
 * 告诉 DDL 当前目标芯片的系列与封装类型。宏值定义见 driver/inc/ddl.h。
 *
 * 目标:HC32F072KA = HC32F072 系列、K 封装(LQFP64/64PIN)。
 */
#ifndef __DDL_DEVICE_H__
#define __DDL_DEVICE_H__

/* 器件系列:HC32F072 */
#define DDL_MCU_SERIES          DDL_DEVICE_SERIES_HC32F072

/* 器件封装:K = 64PIN(LQFP64),与 HC32F072KATA 对应 */
#define DDL_MCU_PACKAGE         DDL_DEVICE_PACKAGE_HC_K

#endif /* __DDL_DEVICE_H__ */
