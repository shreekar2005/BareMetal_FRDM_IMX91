#ifndef STRING_H
#define STRING_H

/**
 * @brief compares two null-terminated strings
 * @param s1 first string to check
 * @param s2 second string to check
 * @return 0 if strings are equal, or the difference between the first non-matching characters
 */
int strcmp(const char *s1, const char *s2);

/**
 * @brief compares two strings up to a maximum number of bytes
 * @param s1 first string
 * @param s2 second string
 * @param n maximum number of characters to compare
 * @return 0 if strings are equal up to n bytes, or the difference between characters
 */
int strncmp(const char *s1, const char *s2, int n);

/**
 * @brief converts a string to an integer, supporting negative numbers
 * @param str the string containing the numbers
 * @return the converted integer value
 */
int atoi(const char *str);

/**
 * @brief calculates the length of a null-terminated string
 * @param s the string to measure
 * @return the number of characters before the null terminator
 */
int strlen(const char *s);

/**
 * @brief copies a string from the source to the destination buffer
 * @param dest the buffer where the string will be copied
 * @param src the null-terminated string to copy
 */
void strcpy(char *dest, const char *src);

/**
 * @brief finds the first occurrence of a substring within a larger string
 * @param haystack the string to be searched
 * @param needle the substring to search for
 * @return pointer to the start of the found substring, or 0/NULL if not found
 */
const char* strstr(const char *haystack, const char *needle);

#endif