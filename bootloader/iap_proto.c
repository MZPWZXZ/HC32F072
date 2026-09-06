/**
 * \file iap_proto.c
 * \brief 串口 IAP 自定义协议实现(Bootloader 侧)
 *
 * 依赖约束(依据官方 FLASH 应用笔记):
 *   - 本镜像位于 0~32K,满足 >32KB 器件对 Flash 擦写代码所在区域的限制;
 *   - 执行扇区擦除时,若 CPU 取值来自 Flash,硬件会自动暂停等待 BUSY,
 *     因此本模块可直接从 Flash 运行。
 */
#include "iap_proto.h"

#include "bsp_uart.h"
#include "ddl.h"        /* Ok/Error、delay1ms */
#include "flash.h"
#include "iap_shared.h"

/* 收帧时,读到帧头后等待剩余字节的超时(毫秒) */
#define IAP_FRAME_RX_TIMEOUT_MS   (1000u)

/* 帧接收缓冲(4B 对齐,便于字写入),扇区缓冲按字对齐声明 */
static uint8_t  s_rx_buf[IAP_FRAME_MAX_LEN] __attribute__((aligned(4)));
static uint32_t s_sector_buf[IAP_FLASH_SECTOR_SIZE / 4u];

/**
 * \brief 带超时的单字节接收
 *
 * \param  timeout_ms 超时毫秒
 * \return int 收到的字节(0~255);-1 超时
 */
static int iap_uart_get_byte(uint32_t timeout_ms)
{
	uint8_t ch;

	while (timeout_ms-- > 0u) {
		if (bsp_uart0_get_char(&ch) != 0) {
			return (int)ch;
		}
		delay1ms(1u);
	}

	return -1;
}

/**
 * \brief CRC16-CCITT-FALSE
 *
 * 多项式 0x1021、初值 0xFFFF、不反算,覆盖 CMD+LEN+PAYLOAD。
 *
 * \param  data 数据指针
 * \param  len  字节数
 * \return uint16_t 校验值
 */
static uint16_t iap_crc16(const uint8_t *data, uint32_t len)
{
	uint16_t crc = 0xFFFFu;
	uint32_t i;
	int bit;

	for (i = 0u; i < len; i++) {
		crc ^= (uint16_t)data[i] << 8u;
		for (bit = 0; bit < 8; bit++) {
			if ((crc & 0x8000u) != 0u) {
				crc = (uint16_t)((uint16_t)(crc << 1u) ^ 0x1021u);
			} else {
				crc = (uint16_t)(crc << 1u);
			}
		}
	}

	return crc;
}

/**
 * \brief CRC32(IEEE,zlib 兼容,与 Python binascii.crc32 一致)
 *
 * \param  data 数据指针
 * \param  len  字节数
 * \return uint32_t 校验值
 */
static uint32_t iap_crc32(const uint8_t *data, uint32_t len)
{
	uint32_t crc = 0xFFFFFFFFu;
	uint32_t i;
	int bit;

	for (i = 0u; i < len; i++) {
		crc ^= data[i];
		for (bit = 0; bit < 8; bit++) {
			if ((crc & 1u) != 0u) {
				crc = (crc >> 1u) ^ 0xEDB88320u;
			} else {
				crc >>= 1u;
			}
		}
	}

	return ~crc;
}

/**
 * \brief 小端读取 16 位
 *
 * \param  p 数据指针
 * \return uint16_t 值
 */
static uint16_t iap_get_le16(const uint8_t *p)
{
	return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8u));
}

/**
 * \brief 小端读取 32 位
 *
 * \param  p 数据指针
 * \return uint32_t 值
 */
static uint32_t iap_get_le32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8u) |
	       ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
}

/**
 * \brief 回复单字节状态
 *
 * \param  status IAP_REPLY_ACK 或 IAP_REPLY_NAK
 * \return None
 */
static void iap_reply(uint8_t status)
{
	bsp_uart0_put_char(status);
}

/**
 * \brief 写入并校验一个扇区(幂等:每次先擦除再写,可安全重试)
 *
 * \param  offset App 区内扇区偏移(512B 对齐)
 * \param  data   512 字节扇区数据
 * \return int 1 成功;0 失败
 */
static int iap_program_sector(uint32_t offset, const uint8_t *data)
{
	uint32_t addr = IAP_APP_BASE + offset;
	const uint32_t *words = (const uint32_t *)data;
	uint32_t i;

	if (Flash_SectorErase(addr) != Ok) {
		return 0;
	}

	for (i = 0u; i < IAP_FLASH_SECTOR_SIZE; i += 4u) {
		if (Flash_WriteWord(addr + i, words[i / 4u]) != Ok) {
			return 0;
		}
	}

	/* 读回校验 */
	for (i = 0u; i < IAP_FLASH_SECTOR_SIZE; i += 4u) {
		if (*(volatile uint32_t *)(addr + i) != words[i / 4u]) {
			return 0;
		}
	}

	return 1;
}

/**
 * \brief 处理 WRITE 命令:写一个扇区
 *
 * \param  payload_len payload 长度(应为 4+512)
 * \return None
 */
static void iap_handle_write(uint16_t payload_len)
{
	const uint8_t *p = &s_rx_buf[IAP_HEADER_LEN];
	uint32_t offset;
	uint32_t i;

	if (payload_len != IAP_WRITE_PAYLOAD_LEN) {
		iap_reply(IAP_REPLY_NAK);
		return;
	}

	offset = iap_get_le32(p);
	if ((offset % IAP_FLASH_SECTOR_SIZE) != 0u ||
	    (offset + IAP_FLASH_SECTOR_SIZE) > IAP_APP_MAX_SIZE) {
		bsp_uart0_write("[iap] bad write offset\r\n");
		iap_reply(IAP_REPLY_NAK);
		return;
	}

	/* 载荷拷入字对齐的扇区缓冲 */
	for (i = 0u; i < IAP_FLASH_SECTOR_SIZE; i++) {
		((uint8_t *)s_sector_buf)[i] = p[4u + i];
	}

	if (iap_program_sector(offset, (const uint8_t *)s_sector_buf) != 0) {
		iap_reply(IAP_REPLY_ACK);
	} else {
		bsp_uart0_write("[iap] write/verify fail\r\n");
		iap_reply(IAP_REPLY_NAK);
	}
}

/**
 * \brief 处理 DONE 命令:整镜像 CRC 校验,通过则跳转 App
 *
 * \param  payload_len payload 长度(应为 8)
 * \return None
 */
static void iap_handle_done(uint16_t payload_len)
{
	const uint8_t *p = &s_rx_buf[IAP_HEADER_LEN];
	uint32_t size;
	uint32_t crc_host;
	uint32_t crc_flash;

	if (payload_len != 8u) {
		iap_reply(IAP_REPLY_NAK);
		return;
	}

	size = iap_get_le32(p);
	crc_host = iap_get_le32(p + 4u);

	if (size == 0u || (size % IAP_FLASH_SECTOR_SIZE) != 0u ||
	    size > IAP_APP_MAX_SIZE) {
		bsp_uart0_write("[iap] bad image size\r\n");
		iap_reply(IAP_REPLY_NAK);
		return;
	}

	/* 从 Flash 回读整镜像并计算 CRC32,与主机端整文件 CRC 比对 */
	crc_flash = iap_crc32((const uint8_t *)IAP_APP_BASE, size);

	if (crc_flash != crc_host) {
		bsp_uart0_write("[iap] crc mismatch, upgrade failed\r\n");
		iap_reply(IAP_REPLY_NAK);
		return;
	}

	bsp_uart0_write("[iap] upgrade ok, jump to app ...\r\n");
	iap_reply(IAP_REPLY_ACK);
	delay1ms(300u);
	iap_jump_to_app();
}

/**
 * \brief 处理 REBOOT 命令:直接跳转 App(不校验 CRC)
 *
 * \param  payload_len payload 长度(应为 0)
 * \return None
 */
static void iap_handle_reboot(uint16_t payload_len)
{
	if (payload_len != 0u) {
		iap_reply(IAP_REPLY_NAK);
		return;
	}

	iap_reply(IAP_REPLY_ACK);
	delay1ms(100u);

	if (iap_app_is_valid() != 0u) {
		iap_jump_to_app();
	} else {
		bsp_uart0_write("[iap] no valid app to boot\r\n");
	}
}

void iap_init(void)
{
	/* 编程时间参数按 HCLK=48MHz(u8FreqCfg=12)配置 */
	while (Flash_Init(12u, TRUE) != Ok) {
		;
	}
}

uint8_t iap_app_is_valid(void)
{
	uint32_t sp;
	uint32_t pc;

	sp = *(volatile uint32_t *)IAP_APP_BASE;
	pc = *(volatile uint32_t *)(IAP_APP_BASE + 4u);

	if ((sp < IAP_RAM_BASE) || (sp >= IAP_RAM_BASE + 0x4000u)) {
		return 0u;
	}

	if ((pc & 1u) == 0u) {
		return 0u;   /* 非 Thumb */
	}
	pc &= ~1u;
	if ((pc < IAP_APP_BASE) ||
	    (pc >= IAP_APP_BASE + IAP_APP_MAX_SIZE)) {
		return 0u;
	}

	return 1u;
}

void iap_jump_to_app(void)
{
	uint32_t pc;

	pc = *(volatile uint32_t *)(IAP_APP_BASE + 4u);

	__disable_irq();
	__set_MSP(*(volatile uint32_t *)IAP_APP_BASE);
	__DSB();
	__ISB();

	((void (*)(void))pc)();   /* pc 含 Thumb 位 */

	for (;;) {
		;   /* 正常不会到达 */
	}
}

int iap_recv_process(uint32_t timeout_ms)
{
	uint32_t pos;
	uint16_t payload_len;
	uint16_t crc_rx;
	int b;

	/* 1. 等待帧头;期间其它字节一律丢弃(容忍任意串口噪声/文本) */
	b = iap_uart_get_byte(timeout_ms);
	if (b < 0) {
		return 0;   /* 超时,无帧 */
	}
	s_rx_buf[0] = (uint8_t)b;

	/* 2. 收取 CMD + LEN */
	for (pos = 1u; pos < IAP_HEADER_LEN; pos++) {
		b = iap_uart_get_byte(IAP_FRAME_RX_TIMEOUT_MS);
		if (b < 0) {
			return 0;   /* 帧未收完整,丢弃 */
		}
		s_rx_buf[pos] = (uint8_t)b;
	}

	payload_len = iap_get_le16(&s_rx_buf[2u]);
	if (payload_len > IAP_WRITE_PAYLOAD_LEN) {
		/* 非法长度:丢弃整帧并 NAK,促使主机重试/中止 */
		iap_reply(IAP_REPLY_NAK);
		return 1;
	}

	/* 3. 收取 payload */
	for (pos = IAP_HEADER_LEN;
	     pos < (uint32_t)IAP_HEADER_LEN + payload_len; pos++) {
		b = iap_uart_get_byte(IAP_FRAME_RX_TIMEOUT_MS);
		if (b < 0) {
			return 0;
		}
		s_rx_buf[pos] = (uint8_t)b;
	}

	/* 4. 收取 CRC16 并校验(覆盖 CMD+LEN+PAYLOAD) */
	crc_rx = 0u;
	for (pos = 0u; pos < IAP_CRC_LEN; pos++) {
		b = iap_uart_get_byte(IAP_FRAME_RX_TIMEOUT_MS);
		if (b < 0) {
			return 0;
		}
		crc_rx |= (uint16_t)((uint16_t)(uint8_t)b << (8u * pos));
	}
	if (iap_crc16(&s_rx_buf[1u], 3u + payload_len) != crc_rx) {
		bsp_uart0_write("[iap] crc16 mismatch\r\n");
		iap_reply(IAP_REPLY_NAK);
		return 1;
	}

	/* 5. 分派命令 */
	switch (s_rx_buf[1u]) {
	case IAP_CMD_SYNC:
		iap_reply(IAP_REPLY_ACK);
		break;

	case IAP_CMD_WRITE:
		iap_handle_write(payload_len);
		break;

	case IAP_CMD_DONE:
		iap_handle_done(payload_len);
		break;

	case IAP_CMD_REBOOT:
		iap_handle_reboot(payload_len);
		break;

	default:
		bsp_uart0_write("[iap] unknown cmd\r\n");
		iap_reply(IAP_REPLY_NAK);
		break;
	}

	return 1;
}
