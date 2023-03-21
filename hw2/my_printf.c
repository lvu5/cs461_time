#define _GNU_SOURCE
#include <inttypes.h>
#include "nano/common.h"
#include <stdarg.h>
#include <stdio.h>
#include "nano/xenSchedule.h"

// override printf with my_printf
void consolePrint(const char *data, int length);

#define MAX_STR_LENGTH 1000

char *itoa(int num, char *str, int base) {
    // Make sure base is valid
    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }

    // Handle negative numbers
    bool is_negative = false;
    if (num < 0) {
        is_negative = true;
        num = -num;
    }

    // Convert to string in reverse order
    int i = 0;
    do {
        int digit = num % base;
        str[i++] = (digit < 10) ? (digit + '0') : (digit - 10 + 'a');
        num /= base;
    } while (num > 0);

    // Add negative sign if necessary
    if (is_negative) {
        str[i++] = '-';
    }

    // Reverse the string
    for (int j = 0; j < i / 2; j++) {
        char temp = str[j];
        str[j] = str[i - j - 1];
        str[i - j - 1] = temp;
    }

    // Add null terminator
    str[i] = '\0';

    return str;
}

void reverse(char *str, int len) {
    int i = 0, j = len - 1;
    while (i < j) {
        char temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++;
        j--;
    }
}

char *ftoa(double num, char *str, int precision) {
    int int_part = (int)num;
    double frac_part = num - int_part;

    // Handle negative numbers
    if (num < 0) {
        *str++ = '-';
        int_part = -int_part;
        frac_part = -frac_part;
    }

    // Convert integer part to string
    int i = 0;
    do {
        str[i++] = (int_part % 10) + '0';
        int_part /= 10;
    } while (int_part > 0);
    reverse(str, i);

    // Add decimal point
    if (precision > 0) {
        str[i++] = '.';
    }

    // Convert fractional part to string
    while (precision > 0) {
        frac_part *= 10;
        int digit = (int)frac_part;
        str[i++] = digit + '0';
        frac_part -= digit;
        precision--;
    }

    // Add null terminator
    str[i] = '\0';

    return str;
}

void format_string(char *buffer, const char *format, ...) {
    va_list args;
    va_start(args, format);

    int buf_pos = 0;
    int fmt_pos = 0;

    while (format[fmt_pos] != '\0') {
        if (format[fmt_pos] == '%') {
            // Parse the format specifier
            fmt_pos++;

            int field_width = 0;
            while (format[fmt_pos] >= '0' && format[fmt_pos] <= '9') {
                field_width = field_width * 10 + (format[fmt_pos] - '0');
                fmt_pos++;
            }

            if (format[fmt_pos] == 's') {
                // String argument
                char *str = va_arg(args, char *);
                int str_len = strlen(str);
                if (field_width > 0 && str_len < field_width) {
                    // Pad with spaces
                    for (int i = 0; i < field_width - str_len; i++) {
                        buffer[buf_pos++] = ' ';
                    }
                }
                for (int i = 0; i < str_len; i++) {
                    buffer[buf_pos++] = str[i];
                }
            } else if (format[fmt_pos] == 'd') {
                // Integer argument
                int num = va_arg(args, int);
                char num_str[MAX_STR_LENGTH];
                int num_len = strlen(itoa(num, num_str, 10));
                if (field_width > 0 && num_len < field_width) {
                    // Pad with zeros
                    for (int i = 0; i < field_width - num_len; i++) {
                        buffer[buf_pos++] = '0';
                    }
                }
                for (int i = 0; i < num_len; i++) {
                    buffer[buf_pos++] = num_str[i];
                }
            } else if (format[fmt_pos] == 'f') {
                // Float argument
                double num = va_arg(args, double);
                char num_str[MAX_STR_LENGTH];
                int num_len = strlen(ftoa(num, num_str, 4));
                if (field_width > 0 && num_len < field_width) {
                    // Pad with zeros
                    for (int i = 0; i < field_width - num_len; i++) {
                        buffer[buf_pos++] = '0';
                    }
                }
                for (int i = 0; i < num_len; i++) {
                    buffer[buf_pos++] = num_str[i];
                }
            } else {
                // Invalid format specifier
                buffer[buf_pos++] = format[fmt_pos];
            }

        } else {
            // Plain text
            buffer[buf_pos++] = format[fmt_pos];
        }

        fmt_pos++;
    }

    // Add null terminator
    buffer[buf_pos] = '\0';

    va_end(args);
}



void my_printf(char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    while (*fmt != '\0') {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 's': {
                    char *str = va_arg(args, char *);
                    if (str == NULL) {
                        char error_msg[100];
                        format_string(error_msg, "Error in file %s on line %d: String argument is NULL", __FILE__, __LINE__);
                        consolePrint(error_msg, strlen(error_msg));
                        // exit the program
                        // exit(3);
                        xenScheduleShutdown(0);
                        va_end(args);
                        return;
                    }
                    consolePrint(str, strlen(str));
                    break;
                }
                case 'd': {
                    int n = va_arg(args, int);
                    char str[100];
                    format_string(str, "%d", n);
                    consolePrint(str, strlen(str));
                    break;
                }
                case 'x': {
                    unsigned int n = va_arg(args, unsigned int);
                    char str[100];
                    format_string(str, "%x", n);
                    consolePrint(str, strlen(str));
                    break;
                }
                case 'u': {
                    unsigned int n = va_arg(args, unsigned int);
                    char str[100];
                    format_string(str, "%u", n);
                    consolePrint(str, strlen(str));
                    break;
                }
                case 'l': {
                    fmt++;
                    switch (*fmt) {
                        case 'd': {
                            long n = va_arg(args, long);
                            char str[100];
                            format_string(str, "%ld", n);
                            consolePrint(str, strlen(str));
                            break;
                        }
                        case 'x': {
                            unsigned long n = va_arg(args, unsigned long);
                            char str[100];
                            format_string(str, "%lx", n);
                            consolePrint(str, strlen(str));
                            break;
                        }
                        case 'u': {
                            unsigned long n = va_arg(args, unsigned long);
                            char str[100];
                            format_string(str, "%lu", n);
                            consolePrint(str, strlen(str));
                            break;
                        }
                        default: {
                            char error_msg[100];
                            format_string(error_msg, "Error in file %s on line %d: String argument is NULL", __FILE__, __LINE__);
                            consolePrint(error_msg, strlen(error_msg));
                            va_end(args);
                            xenScheduleShutdown(0);
                            return;
                        }
                    }
                    break;
                }
                case '%': {
                    consolePrint("%", 1);
                    break;
                }
                default: {
                    char error_msg[100];
                    format_string(error_msg, "Error in file %s on line %d: String argument is NULL", __FILE__, __LINE__);
                    consolePrint(error_msg, strlen(error_msg));
                    va_end(args);
                    // exit(3);
                    xenScheduleShutdown(0);
                    return;
                }
            }
        } else {
            // putchar(*fmt);
            consolePrint(fmt, 1);
        }
        fmt++;
    }
    va_end(args);
}
