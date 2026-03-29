#include "include/string.h"

int my_strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int my_strncmp(const char *s1, const char *s2, int n) {
    while (n && *s1 && (*s1 == *s2)) { s1++; s2++; n--; }
    if (n == 0) return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int my_atoi(const char *str) {
    int res = 0;
    int sign = 1;
    if (*str == '-') {
        sign = -1;
        str++;
    }
    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res * sign;
}

int my_strlen(const char *s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

void my_strcpy(char *dest, const char *src) {
    while (*src) { *dest++ = *src++; }
    *dest = '\0';
}

const char* my_strstr(const char *haystack, const char *needle) {
    if (!*needle) return haystack;
    for (const char *h = haystack; *h; h++) {
        const char *h_iter = h;
        const char *n_iter = needle;
        while (*h_iter && *n_iter && *h_iter == *n_iter) {
            h_iter++;
            n_iter++;
        }
        if (!*n_iter) return h;
    }
    return (const char*)0; 
}