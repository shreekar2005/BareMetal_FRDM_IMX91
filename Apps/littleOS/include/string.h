#ifndef STRING_H
#define STRING_H

/**
 * @brief compares two strings
 * @param s1 first string to check
 * @param s2 second string to check
 * @return difference between first non matching char
 */
int my_strcmp(const char *s1, const char *s2);

/**
 * @brief compares two strings upto n bytes
 * @param s1 first string
 * @param s2 second string
 * @param n number of chars to check
 * @return difference between chars
 */
int my_strncmp(const char *s1, const char *s2, int n);

/**
 * @brief converts string to integer
 * @param str string with numbers
 * @return integer value
 */
int my_atoi(const char *str);

#endif