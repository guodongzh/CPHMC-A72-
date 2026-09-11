/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       syscalls.c
 *@author     zhaoguodong
 *@date       2026.09.11
 *@brief      A72 bare-metal C library system call glue.
 *@par        History
 *Date        Version   Author         Description
 *2026.09.11  1.0       zhaoguodong    Initial version
 ******************************************************************************/

#include <errno.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

extern void UART_putc(unsigned char byteTx);

extern char __heap_start;
extern char __heap_end;

#ifndef STDOUT_FILENO
#define STDOUT_FILENO 1
#endif
#ifndef STDERR_FILENO
#define STDERR_FILENO 2
#endif

static char *g_syscallHeapNext = &__heap_start;

void *_sbrk(ptrdiff_t increment)
{
    char *previous = g_syscallHeapNext;
    char *next = previous + increment;

    if ((next < &__heap_start) || (next > &__heap_end))
    {
        errno = ENOMEM;
        return (void *)-1;
    }

    g_syscallHeapNext = next;
    return previous;
}

int _write(int fileDescriptor, const void *buffer, size_t count)
{
    const unsigned char *data = (const unsigned char *)buffer;
    size_t index;

    if ((fileDescriptor != STDOUT_FILENO) &&
        (fileDescriptor != STDERR_FILENO))
    {
        errno = EBADF;
        return -1;
    }

    for (index = 0U; index < count; index++)
    {
        if (data[index] == (unsigned char)'\n')
        {
            UART_putc((unsigned char)'\r');
        }
        UART_putc(data[index]);
    }

    return (int)count;
}

int _read(int fileDescriptor, void *buffer, size_t count)
{
    (void)fileDescriptor;
    (void)buffer;
    (void)count;
    errno = ENOSYS;
    return -1;
}

int _close(int fileDescriptor)
{
    (void)fileDescriptor;
    errno = EBADF;
    return -1;
}

int _fstat(int fileDescriptor, struct stat *status)
{
    (void)fileDescriptor;

    if (status == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    status->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int fileDescriptor)
{
    return ((fileDescriptor == STDOUT_FILENO) ||
            (fileDescriptor == STDERR_FILENO)) ? 1 : 0;
}

off_t _lseek(int fileDescriptor, off_t offset, int whence)
{
    (void)fileDescriptor;
    (void)offset;
    (void)whence;
    errno = ESPIPE;
    return (off_t)-1;
}

int _getpid(void)
{
    return 1;
}

int _kill(int processId, int signalNumber)
{
    (void)processId;
    (void)signalNumber;
    errno = EINVAL;
    return -1;
}

void _exit(int status)
{
    (void)status;

    for (;;)
    {
        __asm__ volatile("wfe");
    }
}
