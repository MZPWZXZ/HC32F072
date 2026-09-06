/**
 * \file syscalls.c
 * \brief newlib 最小系统调用桩(裸机重定向)
 *
 * 本工程不依赖操作系统,也不使用标准 FILE* 流,但链接 newlib(-nano)时
 * stdio 初始化链会引用 _read/_write/_close/_lseek 等符号;官方库中
 * 对应的“未实现”桩会在链接期打印告警。此处提供最小实现以消除告警:
 *
 *   - _write:UART0 初始化完成后把 stdout 字节送到串口(标准 printf 可用);
 *   - _read / _close / _lseek / _fstat / _isatty:按无终端/无文件处理;
 *   - _sbrk:使用链接脚本中的堆区间(_heap_start ~ _heap_end);
 *   - _exit / _kill / _getpid:崩溃兜底。
 *
 * 每个函数独立成节(-ffunction-sections),未被引用时会被 --gc-sections 裁剪。
 */
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "bsp_uart.h"

/* 链接脚本提供的堆边界(startup/hc32f072ka_flash.ld) */
extern char _heap_start;
extern char _heap_end;

/**
 * \brief 输出重定向:_write() → UART0
 *
 * \param  fd  文件描述符(stdout/stderr 等)
 * \param  buf 数据指针
 * \param  len 字节数
 * \return int 实际输出字节数;UART 未初始化时按 0 处理
 */
int _write(int fd, const char *buf, int len)
{
	(void)fd;

	if (bsp_uart0_is_ready() != 0u) {
		int i;

		for (i = 0; i < len; i++)
			bsp_uart0_put_char((uint8_t)buf[i]);
		return len;
	}

	return 0;
}

/**
 * \brief 输入重定向:本工程无 stdin,直接返回错误
 *
 * \param  fd  文件描述符
 * \param  buf 缓冲区
 * \param  len 期望字节数
 * \return int -1(errno = ENOSYS)
 */
int _read(int fd, char *buf, int len)
{
	(void)fd;
	(void)buf;
	(void)len;

	errno = ENOSYS;
	return -1;
}

/**
 * \brief 关闭文件:裸机无文件系统
 *
 * \param  fd 文件描述符
 * \return int 0(本工程不真正使用 fd)
 */
int _close(int fd)
{
	(void)fd;
	return 0;
}

/**
 * \brief 文件定位:裸机无文件系统
 *
 * \param  fd     文件描述符
 * \param  offset 偏移
 * \param  whence 起点
 * \return int -1(errno = ENOSYS)
 */
int _lseek(int fd, int offset, int whence)
{
	(void)fd;
	(void)offset;
	(void)whence;

	errno = ENOSYS;
	return -1;
}

/**
 * \brief 文件状态:模拟普通字符设备
 *
 * \param  fd 文件描述符
 * \param  st 状态结构指针
 * \return int 0(填充 st_mode = S_IFCHR)
 */
int _fstat(int fd, struct stat *st)
{
	(void)fd;

	if (st != NULL) {
		st->st_mode = S_IFCHR;
		return 0;
	}

	errno = EBADF;
	return -1;
}

/**
 * \brief 是否终端设备:串口可视作终端
 *
 * \param  fd 文件描述符
 * \return int 1
 */
int _isatty(int fd)
{
	(void)fd;
	return 1;
}

/**
 * \brief 扩展堆:newlib malloc 系调用入口
 *
 * \param  incr 申请字节数(可为负)
 * \return void* 新堆顶;越界或失败返回 (void*)-1
 */
void *_sbrk(int incr)
{
	static char *heap_ptr;
	char *old_ptr;

	if (heap_ptr == NULL)
		heap_ptr = &_heap_start;

	old_ptr = heap_ptr;
	if (heap_ptr + incr > &_heap_end || heap_ptr + incr < &_heap_start) {
		errno = ENOMEM;
		return (void *)-1;
	}

	heap_ptr += incr;
	return old_ptr;
}

/**
 * \brief 进程退出:裸机无返回,原地死循环
 *
 * \param  status 退出码(不使用)
 * \return None
 */
void _exit(int status)
{
	(void)status;
	for (;;)
		;
}

/**
 * \brief 信号处理占位:本工程不接收信号
 *
 * \param  pid 进程号
 * \param  sig 信号号
 * \return int -1(errno = ENOSYS)
 */
int _kill(int pid, int sig)
{
	(void)pid;
	(void)sig;

	errno = ENOSYS;
	return -1;
}

/**
 * \brief 进程号:单进程裸机,恒为 1
 *
 * \param  None
 * \return int 1
 */
int _getpid(void)
{
	return 1;
}
