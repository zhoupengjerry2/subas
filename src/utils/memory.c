/*
 * ============================================================================
 * 文件名: memory.c
 * 描述  : 内存管理实现文件。
 * 遵守 C90 规范，手工实现关键的底层操作，为汇编重写铺垫。
 * ============================================================================
 */

#include "../../include/utils.h"
#include "../../include/error.h"
#include <stdlib.h> /* 整个项目中唯一允许出现标准分配器的地方 */

void* util_malloc(u32 size) {
    void* ptr;
    if (size == 0) {
        return NULL_PTR;
    }
    
    ptr = malloc((size_t)size);
    if (ptr == NULL_PTR) {
        /* 在完整的编译器架构中，这里应报告致命错误并可选择退出。
         * 此处安全调用 error_report（包含头文件后并不会产生循环依赖），
         * 便于上层模块感知内存分配失败。 */
        error_report(0, ERR_SYS_OUT_OF_MEM, NULL_PTR);
    }
    return ptr;
}

void util_free(void* ptr) {
    if (ptr != NULL_PTR) {
        free(ptr);
    }
}
