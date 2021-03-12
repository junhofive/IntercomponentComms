/*
 * checksum_generator.c
 *
 *  Created on: Mar 2, 2021
 *      Author: JO_Desktop
 */

#include <str_converter.h>

//https://stackoverflow.com/questions/9631225/convert-strings-specified-by-length-not-nul-terminated-to-int-float
static int i;
static int ret;

int strToInt(const char* str, int len) {
    ret = 0;
    for(i = 0; i < len; ++i)
    {
        ret = ret * 10 + (str[i] - '0');
    }
    return ret;
}

int strToSum(const char* str, int len) {
    ret = 0;
    for(i = 0; i < len; ++i)
    {
        ret += str[i];
    }
    return ret;
}

