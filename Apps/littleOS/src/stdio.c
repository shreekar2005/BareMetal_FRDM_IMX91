#include "include/stdio.h"
#include "LPUART.h"
#include <stdbool.h>

static void printCharStr(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        uart_putchar(LPUART1, str[i]);
    }
}

static void reverse(char *str, int length) {
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

static void ullToString(unsigned long long n, char *buffer, int base, int is_signed, int uppercase) {
    int i = 0;
    int isNegative = 0;
    
    if (n == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return;
    }
    
    if (is_signed && (long long)n < 0) {
        isNegative = 1;
        n = -(long long)n;
    }
    
    while (n != 0) {
        int rem = n % base;
        buffer[i++] = (rem > 9) ? ((rem - 10) + (uppercase ? 'A' : 'a')) : (rem + '0');
        n = n / base;
    }
    
    if (isNegative) buffer[i++] = '-';
    buffer[i] = '\0';
    reverse(buffer, i);
}

static double power(double base, int exp) {
    double res = 1.0;
    for (int i = 0; i < exp; ++i) res *= base;
    return res;
}

static void doubleToString(double d, char *buffer, int precision) {
    if (precision < 0) precision = 6;
    char *ptr = buffer;
    
    if (d < 0) { 
        *ptr++ = '-'; 
        d = -d; 
    }
    
    unsigned long long int_part = (unsigned long long)d;
    double frac_part = d - (double)int_part;

    ullToString(int_part, ptr, 10, 0, 0);
    while (*ptr) ptr++;
    *ptr++ = '.';
    
    unsigned long long frac_as_ull = (unsigned long long)(frac_part * power(10, precision) + 0.5);
    char frac_buffer[32];
    ullToString(frac_as_ull, frac_buffer, 10, 0, 0);

    int frac_len = 0;
    while(frac_buffer[frac_len] != '\0') frac_len++;
    
    int padding = precision - frac_len;
    for (int i = 0; i < padding; i++) *ptr++ = '0';
    
    char* frac_ptr = frac_buffer;
    while(*frac_ptr) *ptr++ = *frac_ptr++;
    *ptr = '\0';
}

static void printHex(uintptr_t n, int digits) {
    char buffer[32];
    ullToString(n, buffer, 16, 0, 1);
    
    int len = 0;
    while (buffer[len] != '\0') len++;
    
    for (int i = 0; i < digits - len; i++) {
        printCharStr("0");
    }
    printCharStr(buffer);
}

int printf(const char *format, ...) {
    int chars_written = 0;
    va_list args;
    va_start(args, format);

    char buffer[128];
    char char_str[2] = {0, 0};

    for (int i = 0; format[i] != '\0'; i++) {
        if (format[i] == '%') {
            i++;
            int use_alternative_form = 0;
            int zero_pad = 0;
            int left_align = 0;
            int width = 0;
            int precision = -1;
            
            while(1) {
                if (format[i] == '-') { left_align = 1; i++; }
                else if (format[i] == '#') { use_alternative_form = 1; i++; }
                else if (format[i] == '0') { zero_pad = 1; i++; }
                else break;
            }
            
            if (left_align) zero_pad = 0;

            while (format[i] >= '0' && format[i] <= '9') {
                width = width * 10 + (format[i] - '0');
                i++;
            }
            
            if (format[i] == '.') {
                i++; 
                precision = 0;
                while (format[i] >= '0' && format[i] <= '9') {
                    precision = precision * 10 + (format[i] - '0');
                    i++;
                }
            }

            int is_long = 0, is_long_long = 0, is_short = 0, is_char = 0;
            
            if (format[i] == 'l') { 
                is_long = 1; i++; 
                if (format[i] == 'l') { is_long_long = 1; is_long = 0; i++; }
            } else if (format[i] == 'h') { 
                is_short = 1; i++; 
                if (format[i] == 'h') { is_char = 1; is_short = 0; i++; }
            }

            switch (format[i]) {
                case 'c': {
                    char_str[0] = (char)va_arg(args, int);
                    printCharStr(char_str);
                    chars_written++;
                    break;
                }
                case 's': {
                    const char *str = va_arg(args, char *);
                    if (!str) str = "(null)";
                    int len = 0; while (str[len]) len++;
                    int padding = (width > len) ? (width - len) : 0;
                    chars_written += len + padding;

                    if (left_align) {
                        printCharStr(str);
                        for(int k=0; k<padding; k++) printCharStr(" ");
                    } else {
                        for(int k=0; k<padding; k++) printCharStr(" ");
                        printCharStr(str);
                    }
                    break;
                }
                case 'f': {
                    doubleToString(va_arg(args, double), buffer, precision);
                    int len = 0; while(buffer[len]) len++;
                    int padding = (width > len) ? (width - len) : 0;
                    chars_written += len + padding;

                    if (left_align) {
                        printCharStr(buffer);
                        for(int k=0; k<padding; k++) printCharStr(" ");
                    } else {
                        char padChar = (zero_pad && precision == -1) ? '0' : ' ';
                        for(int k=0; k<padding; k++) {
                            char p[2] = {padChar, 0};
                            printCharStr(p);
                        }
                        printCharStr(buffer);
                    }
                    break;
                }
                case 'd': case 'i': case 'u': case 'x': case 'X': case 'b': case 'o': {
                    unsigned long long val;
                    int base = 10;
                    int uppercase = 0;
                    char sign_char = 0;

                    if (format[i] == 'd' || format[i] == 'i') {
                        long long signed_val;
                        if (is_long_long) signed_val = va_arg(args, long long);
                        else if (is_long) signed_val = va_arg(args, long);
                        else if (is_char) signed_val = (signed char)va_arg(args, int);
                        else if (is_short) signed_val = (short)va_arg(args, int);
                        else signed_val = va_arg(args, int);
                        
                        if (signed_val < 0) { sign_char = '-'; val = -signed_val; } 
                        else { val = signed_val; }
                    } else {
                        if (is_long_long) val = va_arg(args, unsigned long long);
                        else if (is_long) val = va_arg(args, unsigned long);
                        else if (is_char) val = (unsigned char)va_arg(args, unsigned int);
                        else if (is_short) val = (unsigned short)va_arg(args, unsigned int);
                        else val = va_arg(args, unsigned int);
                    }

                    switch(format[i]) {
                        case 'x': base = 16; break;
                        case 'X': base = 16; uppercase = 1; break;
                        case 'b': base = 2; break;
                        case 'o': base = 8; break;
                    }
                    
                    ullToString(val, buffer, base, 0, uppercase);

                    const char* prefix = "";
                    if (use_alternative_form && val != 0) {
                        switch(format[i]) {
                            case 'x': prefix = "0x"; break;
                            case 'X': prefix = "0X"; break;
                            case 'b': prefix = "0b"; break;
                            case 'o': prefix = "0"; break;
                        }
                    }
                    
                    int num_len = 0; while(buffer[num_len]) num_len++;
                    int prefix_len = 0; while(prefix[prefix_len]) prefix_len++;
                    
                    int precision_pads = (precision > num_len) ? (precision - num_len) : 0;
                    int total_len = num_len + (sign_char ? 1 : 0) + prefix_len + precision_pads;
                    int width_pads = (width > total_len) ? (width - total_len) : 0;
                    
                    chars_written += width_pads + total_len;

                    if (left_align) {
                        if (sign_char) { char_str[0] = sign_char; printCharStr(char_str); }
                        if (prefix_len > 0) printCharStr(prefix);
                        if (precision_pads > 0) {
                            for (int j = 0; j < precision_pads; j++) printCharStr("0");
                        }
                        printCharStr(buffer);
                        for (int j = 0; j < width_pads; j++) printCharStr(" ");
                    } else {
                        if (!zero_pad) {
                            for (int j = 0; j < width_pads; j++) printCharStr(" ");
                        }
                        if (sign_char) { char_str[0] = sign_char; printCharStr(char_str); }
                        if (prefix_len > 0) printCharStr(prefix);
                        if (zero_pad) {
                            for (int j = 0; j < width_pads; j++) printCharStr("0");
                        }
                        if (precision_pads > 0) {
                            for (int j = 0; j < precision_pads; j++) printCharStr("0");
                        }
                        printCharStr(buffer);
                    }
                    break;
                }
                case 'p': {
                    printCharStr("0x");
                    int hex_digits = sizeof(uintptr_t) * 2;
                    printHex((uintptr_t)va_arg(args, void *), hex_digits);
                    chars_written += 2 + hex_digits;
                    break;
                }
                case '%': {
                    printCharStr("%"); 
                    chars_written++; 
                    break;
                }
                default: {
                    printCharStr("%"); 
                    char_str[0] = format[i]; 
                    printCharStr(char_str); 
                    chars_written += 2; 
                    break;
                }
            }
        } else {
            char_str[0] = format[i]; 
            printCharStr(char_str); 
            chars_written++;
        }
    }
    va_end(args);

    return chars_written;
}