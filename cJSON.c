#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "cJSON.h"

/* 错误信息 */
static const char *ep;

const char *cJSON_GetErrorPtr(void) { return ep; }

/* 给 cJSON 定义分配内存和释放内存的函数 */
static void *(*cJSON_malloc)(size_t sz) = malloc;

static void (*cJSON_free)(void *ptr) = free;

/* 设置 cJSON 的内存分配函数 */
void cJSON_InitHooks(cJSON_Hooks *hooks) {
    if (!hooks) { /* Reset hooks */
        cJSON_malloc = malloc;
        cJSON_free = free;
        return;
    }

    cJSON_malloc = (hooks->malloc_fn) ? hooks->malloc_fn : malloc;
    cJSON_free = (hooks->free_fn) ? hooks->free_fn : free;
}

/* 比较两个字符串的大小（不区分大小写） */
static int cJSON_strcasecmp(const char *s1, const char *s2) {
    if (!s1) return (s1 == s2) ? 0 : 1;
    if (!s2) return 1;
    for (; tolower(*s1) == tolower(*s2); ++s1, ++s2) if (*s1 == 0) return 0;
    return tolower(*(const unsigned char *) s1) - tolower(*(const unsigned char *) s2);
}

/* 拷贝字符串，重新分配内存 */
static char *cJSON_strdup(const char *str) {
    size_t len;
    char *copy;

    len = strlen(str) + 1;
    if (!(copy = (char *) cJSON_malloc(len))) return 0;
    memcpy(copy, str, len);
    return copy;
}

/* 创建一个新的 cJSON 对象并分配内存 */
static cJSON *cJSON_New_Item(void) {
    cJSON *node = (cJSON *) cJSON_malloc(sizeof(cJSON));
    if (node) memset(node, 0, sizeof(cJSON));
    return node;
}

/* 释放 cJSON 对象 */
void cJSON_Delete(cJSON *c) {
    cJSON *next;
    while (c) {
        next = c->next;
        if (!(c->type & cJSON_IsReference) && c->child) cJSON_Delete(c->child);
        if (!(c->type & cJSON_IsReference) && c->valuestring) cJSON_free(c->valuestring);
        if (!(c->type & cJSON_StringIsConst) && c->string) cJSON_free(c->string);
        cJSON_free(c);
        c = next;
    }
}

/* 解析一个数字并添加到 cJSON 对象中 */
static const char *parse_number(cJSON *item, const char *num) {
    double n = 0, sign = 1, scale = 0;      // sign 正负号，scale 小数部分位数
    int subscale = 0, signsubscale = 1;     // 科学计数法的指数部分和正负号

    if (!num) return NULL;                  // 无效输入

    if (*num == '-') sign = -1, num++;       // 处理负号
    while (*num == '0') num++;        // 处理 0 开头的数字
    if (*num >= '1' && *num <= '9') {
        do { n = (n * 10.0) + (*num++ - '0'); }
        while (*num >= '0' && *num <= '9'); // 处理整数部分
    }
    if (*num == '.' && num[1] >= '0' && num[1] <= '9') { // 处理小数部分
        num++;
        while (*num >= '0' && *num <= '9') {
            n = (n * 10.0) + (*num++ - '0');
            scale--;
        }
    }
    if (*num == 'E' || *num == 'e') {         // 处理科学计数法
        num++;
        if (*num == '+') num++;
        else if (*num == '-') signsubscale = -1, num++;
        while (*num >= '0' && *num <= '9') subscale = (subscale * 10) + (*num++ - '0');
    }
    n = sign * n * pow(10.0, (scale + subscale * signsubscale)); // 计算最终结果
    item->valuedouble = n;
    item->valueint = (int) n;
    item->type = cJSON_Number;
    return num;
}

/* 找到 >= x 的最小的 2 的幂 */
static size_t pow2gt(size_t x) {
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}

/* 缓冲区结构体 */
typedef struct {
    char *buffer;    // 缓冲区内容
    size_t length;          // 缓冲区长度
    size_t offset;          // 缓冲区偏移
} printbuffer;

/* 检查缓冲区是否足够，不够则重新分配内存 */
static char *ensure(printbuffer *p, size_t needed) {
    char *newbuffer;
    size_t newsize;
    if (!p || !p->buffer) return NULL;
    needed += p->offset;        // 计算新的长度
    if (needed <= p->length) return p->buffer + p->offset; // 如果长度足够，直接返回

    newsize = pow2gt(needed);   // 找到 >= needed 的最小的 2 的幂
    newbuffer = (char *) cJSON_malloc(newsize); // 重新分配内存
    if (!newbuffer) {
        cJSON_free(p->buffer);
        p->length = 0, p->buffer = 0;
        return NULL;
    }
    if (newbuffer) memcpy(newbuffer, p->buffer, p->length); // 拷贝原来的内容
    cJSON_free(p->buffer); // 释放原来的内存
    p->length = newsize;    // 更新长度
    p->buffer = newbuffer;  // 更新内容
    return p->buffer + p->offset; // 返回新的偏移
}

/* 更新缓冲区的偏移 */
static size_t update_offset(printbuffer *p) {
    if (!p || !p->buffer) return 0;
    p->offset += strlen(p->buffer + p->offset); // 更新偏移
    return p->offset;
}

/* 输出一个 cJSON 对象的数字部分到缓冲区 */
static char *print_number(cJSON *item, printbuffer *p) {
    char *str = NULL;
    double d = item->valuedouble;
    if (d == 0) {
        if (p) str = ensure(p, 2);
        else str = (char *) cJSON_malloc(2);
        if (str) strcpy(str, "0");
    } else if (fabs(((double) item->valueint) - d) <= DBL_EPSILON && d <= INT_MAX && d >= INT_MIN) {
        if (p) str = ensure(p, 21);
        else str = (char *) cJSON_malloc(21);
        if (str) sprintf(str, "%d", item->valueint);
    } else {
        if (p) str = ensure(p, 64);
        else str = (char *) cJSON_malloc(64);
        if (str) {
            if (fabs(floor(d) - d) <= DBL_EPSILON && fabs(d) < 1.0e60) sprintf(str, "%.0f", d);
            else if (fabs(d) < 1.0e-6 || fabs(d) > 1.0e9) sprintf(str, "%e", d);
            else sprintf(str, "%f", d);
        }
    }
    return str;
}

static unsigned parse_hex4(const char *str) {
    unsigned h = 0, i;
    for (i = 0; i < 4; i++) {
        h <<= 4;
        if (str[i] >= '0' && str[i] <= '9') h |= str[i] - '0';
        else if (str[i] >= 'A' && str[i] <= 'F') h |= str[i] - ('A' - 10);
        else if (str[i] >= 'a' && str[i] <= 'f') h |= str[i] - ('a' - 10);
        else return 0;
    }
    return h;
}

/* 解析一个字符串并添加到 cJSON 对象中 */
static const unsigned char firstByteMark[7] = {0, 0, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};     // UTF-8 编码的第一个字节的掩码
static const char *parse_string(cJSON *item, const char *str) {
    const char *ptr = str + 1;
    char *ptr2;
    char *out;
    int len = 0;
    unsigned uc, uc2;
    if (*str != '\"') { ep = str; return NULL; } // 非法输入，不是一个字符串

    while (*ptr != '\"' && *ptr && ++len) if (*ptr++ == '\\') ptr++; // 计算字符串长度
    if (*ptr != '\"') { ep = ptr; return NULL; } // 非法输入，没有找到字符串的结束

    out = (char *) cJSON_malloc(len + 1); // 分配内存
    if (!out) return NULL;

    ptr = str + 1; ptr2 = out; // 重新指向字符串
    while (*ptr != '\"' && *ptr) {
        if (*ptr != '\\') *ptr2++ = *ptr++;
        else {
            ptr++;
            switch (*ptr) {
                case 'b': *ptr2++ = '\b'; break;
                case 'f': *ptr2++ = '\f'; break;
                case 'n': *ptr2++ = '\n'; break;
                case 'r': *ptr2++ = '\r'; break;
                case 't': *ptr2++ = '\t'; break;
                case 'u': {                // 处理 Unicode 编码
                    uc = parse_hex4(ptr + 1); ptr += 4;         // 解析 4 位的 Unicode 编码

                    if ((uc >= 0xDC00 && uc <= 0xDFFF) || uc == 0) break; // 无效的 Unicode 编码（低位代理项）

                    if (uc >= 0xD800 && uc <= 0xDBFF) {          // 高位代理项
                        if (ptr[1] != '\\' || ptr[2] != 'u') break; // 缺少低位代理项
                        uc2 = parse_hex4(ptr + 3); ptr += 6;       // 解析低位代理项
                        if (uc2 < 0xDC00 || uc2 > 0xDFFF) break;   // 无效的 Unicode 编码（高位代理项）
                        uc = 0x10000 + (((uc & 0x3FF) << 10) | (uc2 & 0x3FF)); // 合并高位和低位代理项
                    }

                    len = 4;
                    if (uc < 0x80) len = 1;         // 1 字节
                    else if (uc < 0x800) len = 2;   // 2 字节
                    else if (uc < 0x10000) len = 3; // 3 字节
                    ptr2 += len;

                    switch (len) {
                        case 4: *--ptr2 = ((uc | 0x80) & 0xBF); uc >>= 6;
                        case 3: *--ptr2 = ((uc | 0x80) & 0xBF); uc >>= 6;
                        case 2: *--ptr2 = ((uc | 0x80) & 0xBF); uc >>= 6;
                        case 1: *--ptr2 = (uc | firstByteMark[len]);
                        default: break;
                    }
                    ptr2 += len;
                    break;
                }
                default: *ptr2++ = *ptr; break;
            }
            ptr++;
        }
    }
    *ptr2 = 0;
    if (*ptr == '\"') ptr++;
    item->valuestring = out;
    item->type = cJSON_String;
    return ptr;
}

cJSON *cJSON_ParseWithOpts(const char *value, const char **return_parse_end, cJSON_bool require_null_terminated) {

}