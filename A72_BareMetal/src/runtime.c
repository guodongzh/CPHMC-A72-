/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       runtime.c
 *@author     zhaoguodong
 *@date       2026.09.11
 *@brief      Minimal freestanding C runtime helpers.
 *@par        History
 *Date        Version   Author         Description
 *2026.09.11  1.0       zhaoguodong    Initial version
 ******************************************************************************/

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

/*
 * The bare-metal link intentionally omits libc.  TI PDK driver sources use
 * structure assignment, which GCC lowers to memcpy for larger objects.
 */
void *memcpy(void *destination, const void *source, size_t count)
{
    unsigned char *dst = (unsigned char *)destination;
    const unsigned char *src = (const unsigned char *)source;

    while (count != 0U)
    {
        *dst++ = *src++;
        --count;
    }
    return destination;
}

void *memset(void *destination, int value, size_t count)
{
    unsigned char *dst = (unsigned char *)destination;

    while (count != 0U)
    {
        *dst++ = (unsigned char)value;
        --count;
    }
    return destination;
}

char *strncpy(char *destination, const char *source, size_t count)
{
    char *dst = destination;

    while (count != 0U)
    {
        if (*source != '\0')
        {
            *dst++ = *source++;
        }
        else
        {
            *dst++ = '\0';
        }
        --count;
    }
    return destination;
}

static void runtime_format_putc(char *destination,
                                size_t destinationSize,
                                size_t *characterCount,
                                char value)
{
    if ((destinationSize != 0U) && (*characterCount < (destinationSize - 1U)))
    {
        destination[*characterCount] = value;
    }
    *characterCount += 1U;
}

static void runtime_format_unsigned(char *destination,
                                    size_t destinationSize,
                                    size_t *characterCount,
                                    uint64_t value,
                                    uint32_t base,
                                    uint32_t minimumWidth,
                                    char padCharacter,
                                    uint32_t upperCase)
{
    char numberBuffer[24U];
    const char *digits;
    uint32_t index = 0U;

    digits = (upperCase != 0U) ? "0123456789ABCDEF" : "0123456789abcdef";
    do
    {
        numberBuffer[index] = digits[value % base];
        value /= base;
        index++;
    } while (value != 0U);

    while (index < minimumWidth)
    {
        runtime_format_putc(destination,
                            destinationSize,
                            characterCount,
                            padCharacter);
        minimumWidth--;
    }
    while (index != 0U)
    {
        index--;
        runtime_format_putc(destination,
                            destinationSize,
                            characterCount,
                            numberBuffer[index]);
    }
}

/*
 * TI UDMA's optional diagnostics use vsnprintf even when diagnostics are not
 * enabled by the application.  Keep a small freestanding implementation here
 * instead of pulling a full C runtime into the A72 image.
 */
int vsnprintf(char *destination,
              size_t destinationSize,
              const char *format,
              va_list arguments)
{
    size_t characterCount = 0U;

    while (*format != '\0')
    {
        uint32_t minimumWidth = 0U;
        uint32_t zeroPad = 0U;
        uint32_t longCount = 0U;
        char conversion;

        if (*format != '%')
        {
            runtime_format_putc(destination,
                                destinationSize,
                                &characterCount,
                                *format++);
            continue;
        }

        format++;
        if (*format == '0')
        {
            zeroPad = 1U;
            format++;
        }
        while ((*format >= '0') && (*format <= '9'))
        {
            minimumWidth = (minimumWidth * 10U) + (uint32_t)(*format - '0');
            format++;
        }
        while (*format == 'l')
        {
            longCount++;
            format++;
        }

        conversion = *format;
        if (conversion == '\0')
        {
            break;
        }
        format++;

        if ((conversion == 'd') || (conversion == 'i'))
        {
            int64_t signedValue;
            uint64_t unsignedValue;

            if (longCount > 1U)
            {
                signedValue = va_arg(arguments, long long);
            }
            else if (longCount != 0U)
            {
                signedValue = va_arg(arguments, long);
            }
            else
            {
                signedValue = va_arg(arguments, int);
            }
            if (signedValue < 0)
            {
                runtime_format_putc(destination,
                                    destinationSize,
                                    &characterCount,
                                    '-');
                unsignedValue = (uint64_t)(-(signedValue + 1)) + 1U;
            }
            else
            {
                unsignedValue = (uint64_t)signedValue;
            }
            runtime_format_unsigned(destination,
                                    destinationSize,
                                    &characterCount,
                                    unsignedValue,
                                    10U,
                                    minimumWidth,
                                    (zeroPad != 0U) ? '0' : ' ',
                                    0U);
        }
        else if ((conversion == 'u') || (conversion == 'x') ||
                 (conversion == 'X'))
        {
            uint64_t unsignedValue;
            uint32_t base = (conversion == 'u') ? 10U : 16U;

            if (longCount > 1U)
            {
                unsignedValue = va_arg(arguments, unsigned long long);
            }
            else if (longCount != 0U)
            {
                unsignedValue = va_arg(arguments, unsigned long);
            }
            else
            {
                unsignedValue = va_arg(arguments, unsigned int);
            }
            runtime_format_unsigned(destination,
                                    destinationSize,
                                    &characterCount,
                                    unsignedValue,
                                    base,
                                    minimumWidth,
                                    (zeroPad != 0U) ? '0' : ' ',
                                    (conversion == 'X') ? 1U : 0U);
        }
        else if (conversion == 'p')
        {
            runtime_format_putc(destination,
                                destinationSize,
                                &characterCount,
                                '0');
            runtime_format_putc(destination,
                                destinationSize,
                                &characterCount,
                                'x');
            runtime_format_unsigned(destination,
                                    destinationSize,
                                    &characterCount,
                                    (uint64_t)(uintptr_t)va_arg(arguments, void *),
                                    16U,
                                    (uint32_t)(sizeof(uintptr_t) * 2U),
                                    '0',
                                    0U);
        }
        else if (conversion == 's')
        {
            const char *stringValue = va_arg(arguments, const char *);

            if (stringValue == (const char *)0)
            {
                stringValue = "(null)";
            }
            while (*stringValue != '\0')
            {
                runtime_format_putc(destination,
                                    destinationSize,
                                    &characterCount,
                                    *stringValue++);
            }
        }
        else if (conversion == 'c')
        {
            runtime_format_putc(destination,
                                destinationSize,
                                &characterCount,
                                (char)va_arg(arguments, int));
        }
        else if (conversion == '%')
        {
            runtime_format_putc(destination,
                                destinationSize,
                                &characterCount,
                                '%');
        }
        else
        {
            runtime_format_putc(destination,
                                destinationSize,
                                &characterCount,
                                '%');
            runtime_format_putc(destination,
                                destinationSize,
                                &characterCount,
                                conversion);
        }
    }

    if (destinationSize != 0U)
    {
        destination[(characterCount < (destinationSize - 1U)) ?
                    characterCount : (destinationSize - 1U)] = '\0';
    }

    return (int)characterCount;
}
