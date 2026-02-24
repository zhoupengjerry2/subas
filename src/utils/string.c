/*
 * ============================================================================
 * 文件名: string.c
 * 描述  : 字符串处理实现文件。
 * 遵守 C90 规范，手工实现关键的底层操作，为汇编重写铺垫。
 * ============================================================================
 */

#include "../../include/utils.h"


u32 util_strlen(const char* str) {
    const char* p = str;
    while (*p != '\0') {
        p++;
    }
    return (u32)(p - str);
}

char* util_strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++) != '\0') {
        /* 空循环体：赋值和自增均在条件中完成 */
    }
    return dest;
}

s32 util_strcmp(const char* s1, const char* s2) {
    while (*s1 != '\0' && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char* util_strdup(const char* str) {
    u32 len;
    char* copy;
    
    if (str == NULL_PTR) {
        return NULL_PTR;
    }
    
    len = util_strlen(str) + 1;
    copy = (char*)util_malloc(len);
    if (copy != NULL_PTR) {
        util_strcpy(copy, str);
    }
    return copy;
}

void* util_memset(void* ptr, int value, u32 size) {
    u8* p = (u8*)ptr;
    u8 byte_value = (u8)(value & 0xFF);
    u32 i;
    
    for (i = 0; i < size; i++) {
        p[i] = byte_value;
    }
    
    return ptr;
}

