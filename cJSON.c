#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <limits.h>
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

/**
 * @brief 解析一个数字并添加到 cJSON 对象中
 *
 * @param item cJSON 对象，用于存储解析后的数字
 * @param num 指向数字字符串的指针
 * @return const char* 成功时返回下一个要解析的位置，失败时返回 NULL
 */static const char *parse_number(cJSON *item, const char *num) {
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

/**
 * @brief 检查缓冲区是否足够，不够则重新分配内存
 *
 * @param p 指向 printbuffer 的指针
 * @param needed 需要的额外空间大小
 * @return char* 成功时返回新的缓冲区指针，失败时返回 NULL
 */static char *ensure(printbuffer *p, size_t needed) {
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
    memcpy(newbuffer, p->buffer, p->length); // 拷贝原来的内容
    cJSON_free(p->buffer); // 释放原来的内存
    p->length = newsize;    // 更新长度
    p->buffer = newbuffer;  // 更新内容
    return p->buffer + p->offset; // 返回新的偏移
}

/**
 * @brief 更新缓冲区的偏移
 *
 * @param p 指向 printbuffer 的指针
 * @return size_t 返回更新后的偏移量
 */static size_t update_offset(printbuffer *p) {
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
            else
                sprintf(str, "%f", d);
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
/**
 * @brief 解析 JSON 字符串值
 *
 * @param item cJSON 对象，用于存储解析后的字符串
 * @param str 指向 JSON 字符串的指针
 * @return const char* 成功时返回下一个要解析的位置，失败时返回 NULL
 */
static const char *parse_string(cJSON *item, const char *str) {
    const char *ptr = str + 1;
    char *ptr2;
    char *out;
    int len = 0;
    unsigned uc, uc2;
    if (*str != '\"') {
        ep = str;
        return NULL;
    } // 非法输入，不是一个字符串

    while (*ptr != '\"' && *ptr && ++len) if (*ptr++ == '\\') ptr++; // 计算字符串长度
    if (*ptr != '\"') {
        ep = ptr;
        return NULL;
    } // 非法输入，没有找到字符串的结束

    out = (char *) cJSON_malloc(len + 1); // 分配内存
    if (!out) return NULL;

    ptr = str + 1;
    ptr2 = out; // 重新指向字符串
    while (*ptr != '\"' && *ptr) {
        if (*ptr != '\\') *ptr2++ = *ptr++;
        else {
            ptr++;
            switch (*ptr) {
                case 'b':
                    *ptr2++ = '\b';
                    break;
                case 'f':
                    *ptr2++ = '\f';
                    break;
                case 'n':
                    *ptr2++ = '\n';
                    break;
                case 'r':
                    *ptr2++ = '\r';
                    break;
                case 't':
                    *ptr2++ = '\t';
                    break;
                case 'u': {                // 处理 Unicode 编码
                    uc = parse_hex4(ptr + 1);
                    ptr += 4;         // 解析 4 位的 Unicode 编码

                    if ((uc >= 0xDC00 && uc <= 0xDFFF) || uc == 0) break; // 无效的 Unicode 编码（低位代理项）

                    if (uc >= 0xD800 && uc <= 0xDBFF) {          // 高位代理项
                        if (ptr[1] != '\\' || ptr[2] != 'u') break; // 缺少低位代理项
                        uc2 = parse_hex4(ptr + 3);
                        ptr += 6;       // 解析低位代理项
                        if (uc2 < 0xDC00 || uc2 > 0xDFFF) break;   // 无效的 Unicode 编码（高位代理项）
                        uc = 0x10000 + (((uc & 0x3FF) << 10) | (uc2 & 0x3FF)); // 合并高位和低位代理项
                    }

                    len = 4;
                    if (uc < 0x80) len = 1;         // 1 字节
                    else if (uc < 0x800) len = 2;   // 2 字节
                    else if (uc < 0x10000) len = 3; // 3 字节
                    ptr2 += len;

                    switch (len) {
                        case 4:
                            *--ptr2 = (char) ((uc | 0x80) & 0xBF);
                            uc >>= 6;
                        case 3:
                            *--ptr2 = (char) ((uc | 0x80) & 0xBF);
                            uc >>= 6;
                        case 2:
                            *--ptr2 = (char) ((uc | 0x80) & 0xBF);
                            uc >>= 6;
                        case 1:
                            *--ptr2 = (char) (uc | firstByteMark[len]);
                        default:
                            break;
                    }
                    ptr2 += len;
                    break;
                }
                default:
                    *ptr2++ = *ptr;
                    break;
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

/**
 * @brief 将提供的 string 渲染为可以打印的转义版本，返回新的字符串
 *
 * @param str 指向要打印的字符串的指针
 * @param p 指向 printbuffer 的指针，用于存储转换后的字符串
 * @return char* 成功时返回转换后的字符串，失败时返回 NULL
 */static char *print_string_ptr(const char *str, printbuffer *p) {
    const char *ptr;
    char *ptr2, *out;
    size_t len = 0;
    int flag = 0;
    unsigned char token;

    if (!str) {
        if (p) out = ensure(p, 3);          // 3 个字符：左右引号和 \0
        else out = (char *) cJSON_malloc(3);
        if (!out) return NULL;
        strcpy(out, "\"\"");
        return out;
    }

    for (ptr = str; *ptr; ptr++)
        flag |= ((*ptr > 0 && *ptr < 32) || (*ptr == '\"') || (*ptr == '\\'));   // ASCII 控制字符、引号、反斜杠

    if (!flag) {
        len = ptr - str;

        if (p) out = ensure(p, len + 3);
        else out = (char *) cJSON_malloc(len + 3);

        if (!out) return NULL;
        ptr2 = out;
        *ptr2++ = '\"';
        strcpy(ptr2, str);
        ptr2[len] = '\"';
        ptr2[len + 1] = 0;

        return out;
    }

    ptr = str;
    while ((token = *ptr) && ++len) {       // 计算转义后的字符串长度，\X 多占 1 个字符 以及 ASCII 控制字符 \uXXXX 多占用 5 个字符
        if (strchr("\"\\\b\f\n\r\t", token)) len++;
        else if (token < 32) len += 5;
        ptr++;
    }

    if (p) out = ensure(p, len + 3);
    else out = (char *) cJSON_malloc(len + 3);

    if (!out) return NULL;

    ptr2 = out;
    ptr = str;
    *ptr2++ = '\"';
    while (*ptr) {
        if ((unsigned char) *ptr > 31 && *ptr != '\"' && *ptr != '\\') *ptr2++ = *ptr++;
        else {
            *ptr2++ = '\\';
            switch (token = *ptr++) {
                case '\\':
                    *ptr2++ = '\\';
                    break;
                case '\"':
                    *ptr2++ = '\"';
                    break;
                case '\b':
                    *ptr2++ = 'b';
                    break;
                case '\f':
                    *ptr2++ = 'f';
                    break;
                case '\n':
                    *ptr2++ = 'n';
                    break;
                case '\r':
                    *ptr2++ = 'r';
                    break;
                case '\t':
                    *ptr2++ = 't';
                    break;
                default:
                    sprintf(ptr2, "u%04x", token);
                    ptr2 += 5;
                    break;
            }
        }
    }
    *ptr2++ = '\"';
    *ptr2 = 0;
    return out;
}

/**
 * @brief 利用 print_string_ptr 将 cJSON 对象的字符串打印到缓冲区
 *
 * @param item cJSON 对象
 * @param p 指向 printbuffer 的指针，用于存储转换后的字符串
 * @return char* 成功时返回转换后的字符串，失败时返回 NULL
 */static char *print_string(cJSON *item, printbuffer *p) {
    return print_string_ptr(item->valuestring, p);
}

/**
 * @brief 跳过空白字符
 *
 * @param in 指向字符串的指针
 * @return const char* 返回跳过空白字符后的指针位置
 */
static const char *skip(const char *in) {
    while (in && *in && (unsigned char) *in <= 32) in++;
    return in;
}

/* 先声明核心解析/输出函数 */
static const char *parse_value(cJSON *item, const char *value);

static const char *parse_array(cJSON *item, const char *value);

static const char *parse_object(cJSON *item, const char *value);

static char *print_value(cJSON *item, int depth, cJSON_bool fmt, printbuffer *p);

static char *print_array(cJSON *item, int depth, cJSON_bool fmt, printbuffer *p);

static char *print_object(cJSON *item, int depth, cJSON_bool fmt, printbuffer *p);

/**
 * @brief 使用可选参数解析 JSON 字符串并创建 cJSON 对象
 *
 * @param value 指向 JSON 字符串的指针
 * @param return_parse_end 可选参数，用于返回解析结束的位置
 * @param require_null_terminated 如果为真，则要求 JSON 字符串以 null 结尾
 * @return cJSON* 成功时返回新创建的 cJSON 对象，失败时返回 NULL
 */
cJSON *cJSON_ParseWithOpts(const char *value, const char **return_parse_end, cJSON_bool require_null_terminated) {
    const char *end = NULL;
    cJSON *c = cJSON_New_Item();
    ep = NULL;
    if (!c) return NULL; /* 内存分配失败 */

    end = parse_value(c, skip(value));
    if (!end) {
        cJSON_Delete(c);
        return NULL;
    } /* 解析失败 */

    /* 如果需要以 \0 结尾，则检查是否以 \0 结尾 */
    if (require_null_terminated) {
        end = skip(end);
        if (*end) {
            cJSON_Delete(c);
            ep = end;
            return NULL;
        }
    }
    if (return_parse_end) *return_parse_end = end;
    return c;
}

/**
 * @brief 解析一个 JSON 值并将其添加到 cJSON 对象中
 *
 * @param item cJSON 对象
 * @param value 指向 JSON 字符串的指针
 * @return const char* 成功时返回下一个要解析的位置，失败时返回 NULL
 */
static const char *parse_value(cJSON *item, const char *value) {
    if (!value) return NULL; // 无效输入

    if (!strncmp(value, "null", 4)) {       // 解析 null
        item->type = cJSON_NULL;
        return value + 4;
    }

    if (!strncmp(value, "false", 5)) {      // 解析 false
        item->type = cJSON_False;
        return value + 5;
    }

    if (!strncmp(value, "true", 5)) {
        item->type = cJSON_True;            // 解析 true
        item->valueint = 1;
        return value + 4;
    }

    if (*value == '\"') {                   // 解析字符串
        return parse_string(item, value);
    }

    if (*value == '-' || (*value >= '0' && *value <= '9')) { // 解析数字
        return parse_number(item, value);
    }

    if (*value == '[') {
        return parse_array(item, value);
    }

    if (*value == '{') {
        return parse_object(item, value);
    }

    ep = value;
    return NULL;                        // 无法解析
}

/**
 * @brief 解析 JSON 数组
 *
 * @param item cJSON 对象
 * @param value 指向 JSON 字符串的指针
 * @return const char* 成功时返回下一个要解析的位置，失败时返回 NULL
 */
static const char *parse_array(cJSON *item, const char *value) {
    cJSON *child;
    if (*value != '[') {
        ep = value;
        return NULL;
    } // 非法输入

    item->type = cJSON_Array;
    value = skip(value + 1);
    if (*value == ']') return value + 1; // 空数组

    item->child = child = cJSON_New_Item();
    if (!item->child) return NULL; // 内存分配失败

    value = skip(parse_value(child, skip(value)));
    if (!value) return NULL; // 解析失败

    while (*value == ',') {
        cJSON *new_item;
        if (!(new_item = cJSON_New_Item())) return NULL; // 内存分配失败
        child->next = new_item, new_item->prev = child, child = new_item;
        value = skip(parse_value(child, skip(value + 1)));
        if (!value) return NULL; // 解析失败
    }

    if (*value == ']') return value + 1; // 解析成功
    ep = value;
    return NULL; // 解析失败
}

/**
 * @brief 解析 JSON 对象
 *
 * @param item cJSON 对象
 * @param value 指向 JSON 字符串的指针
 * @return const char* 成功时返回下一个要解析的位置，失败时返回 NULL
 */static const char *parse_object(cJSON *item, const char *value) {
    cJSON *child;
    if (*value != '{') {
        ep = value;
        return NULL;
    } // 非法输入

    item->type = cJSON_Object;
    value = skip(value + 1);
    if (*value == '}') return value + 1; // 空对象

    item->child = child = cJSON_New_Item();
    if (!item->child) return NULL; // 内存分配失败

    value = skip(parse_string(child, skip(value)));
    if (!value) return NULL; // 解析失败

    child->string = child->valuestring;
    child->valuestring = NULL;

    if (*value != ':') {
        ep = value;
        return NULL;
    } // 非法输入

    value = skip(parse_value(child, skip(value + 1)));
    if (!value) return NULL; // 解析失败

    while (*value == ',') {
        cJSON *new_item;

        if (!(new_item = cJSON_New_Item())) return NULL; // 内存分配失败

        child->next = new_item, new_item->prev = child, child = new_item;

        value = skip(parse_string(child, skip(value + 1)));
        if (!value) return NULL; // 解析失败

        child->string = child->valuestring;
        child->valuestring = NULL;

        if (*value != ':') {
            ep = value;
            return NULL;
        } // 非法输入

        value = skip(parse_value(child, skip(value + 1)));
        if (!value) return NULL; // 解析失败
    }

    if (*value == '}') return value + 1; // 解析成功
    ep = value;
    return NULL; // 解析失败
}

/**
 * @brief 将 cJSON 对象转换为 JSON 字符串
 *
 * @param item cJSON 对象
 * @param depth 当前对象的嵌套深度
 * @param fmt 是否格式化输出
 * @param p 指向 printbuffer 的指针，用于存储转换后的字符串
 * @return char* 成功时返回转换后的字符串，失败时返回 NULL
 */static char *print_value(cJSON *item, int depth, int fmt, printbuffer *p) {
    char *out = NULL;
    if (!item) return NULL;

    if (p) {
        switch ((item->type) & 255) {
            case cJSON_NULL: {
                out = ensure(p, 5);
                if (out) strcpy(out, "null");
                break;
            }
            case cJSON_False: {
                out = ensure(p, 6);
                if (out) strcpy(out, "false");
                break;
            }
            case cJSON_True: {
                out = ensure(p, 5);
                if (out) strcpy(out, "true");
                break;
            }
            case cJSON_Number: {
                out = print_number(item, p);
                break;
            }
            case cJSON_String: {
                out = print_string(item, p);
                break;
            }
            case cJSON_Array: {
                out = print_array(item, depth, fmt, p);
                break;
            }
            case cJSON_Object: {
                out = print_object(item, depth, fmt, p);
                break;
            }
            default:
                break;
        }
    } else {
        switch ((item->type) & 255) {
            case cJSON_NULL: {
                out = cJSON_strdup("null");
                break;
            }
            case cJSON_False: {
                out = cJSON_strdup("false");
                break;
            }
            case cJSON_True: {
                out = cJSON_strdup("true");
                break;
            }
            case cJSON_Number: {
                out = print_number(item, 0);
                break;
            }
            case cJSON_String: {
                out = print_string(item, 0);
                break;
            }
            case cJSON_Array: {
                out = print_array(item, depth, fmt, 0);
                break;
            }
            case cJSON_Object: {
                out = print_object(item, depth, fmt, 0);
                break;
            }
            default:
                break;
        }
    }
    return out;
}

/**
 * @brief 将 cJSON 数组转换为 JSON 字符串
 *
 * @param item cJSON 对象
 * @param depth 当前对象的嵌套深度
 * @param fmt 是否格式化输出
 * @param p 指向 printbuffer 的指针，用于存储转换后的字符串
 * @return char* 成功时返回转换后的字符串，失败时返回 NULL
 */
static char *print_array(cJSON *item, int depth, int fmt, printbuffer *p) {
    char **entries;
    char *out = NULL, *ptr, *ret;
    size_t len = 5;
    cJSON *child = item->child;
    size_t numentries = 0, i = 0, fail = 0;
    size_t tmplen = 0;

    while (child) numentries++, child = child->next;    // 计算数组元素个数

    if (!numentries) {
        if (p) out = ensure(p, 3);
        else out = (char *) cJSON_malloc(3);
        if (out) strcpy(out, "[]");
        return out;
    }

    if (p) {    // 为数组元素分配内存
        i = p->offset;
        ptr = ensure(p, 1);
        if (!ptr) return NULL;
        *ptr = '[';
        p->offset++;
        child = item->child;

        while (child && !fail) {
            print_value(child, depth + 1, fmt, p);
            p->offset = update_offset(p);
            if (child->next) {  // 添加逗号
                len = fmt ? 2 : 1;          // 逗号和空格
                ptr = ensure(p, len + 1);
                if (!ptr) return NULL;
                *ptr++ = ',';
                if (fmt) *ptr++ = ' ';
                *ptr = 0;
                p->offset += len;
            }
            child = child->next;
        }

        ptr = ensure(p, 2);
        if (!ptr) return NULL;
        *ptr++ = ']';
        *ptr = 0;
        out = p->buffer + i;
    } else {
        // 为数组元素分配内存
        entries = (char **) cJSON_malloc(numentries * sizeof(char *));
        if (!entries) return NULL;
        memset(entries, 0, numentries * sizeof(char *));

        child = item->child;
        while (child && !fail) {
            ret = print_value(child, depth + 1, fmt, 0);
            entries[i++] = ret;
            if (ret) len += strlen(ret) + 2 + (fmt ? 1 : 0);        // 字符串长度 + 逗号 + 空格
            else fail = 1;
            child = child->next;
        }

        if (!fail) out = (char *) cJSON_malloc(len);
        if (!out) fail = 1;

        if (fail) {
            for (i = 0; i < numentries; i++) if (entries[i]) cJSON_free(entries[i]);
            cJSON_free(entries);
            return NULL;
        }

        *out = '[';
        ptr = out + 1;
        *ptr = 0;
        for (i = 0; i < numentries; i++) {
            tmplen = strlen(entries[i]);
            memcpy(ptr, entries[i], tmplen);
            ptr += tmplen;
            if (i != numentries - 1) {      // 添加逗号
                *ptr++ = ',';
                if (fmt) {
                    *ptr++ = ' ';
                }
                *ptr = 0;
            }
            cJSON_free(entries[i]);
        }
        cJSON_free(entries);
        *ptr++ = ']';
        *ptr++ = 0;
    }
    return out;
}

static char *print_object(cJSON *item, int depth, cJSON_bool fmt, printbuffer *p) {
    char **entries = NULL, **names = NULL;
    char *out = NULL, *ptr, *ret, *str;
    size_t len = 7, i = 0, j;
    cJSON *child = item->child;
    int numentries = 0, fail = 0;
    size_t tmplen = 0;

    while (child) numentries++, child = child->next;    // 计算对象元素个数

    if (!numentries) {
        if (p) out = ensure(p, 2);
        else out = (char *) cJSON_malloc(2);
        if (!out) return NULL;
        ptr = out;
        *ptr++ = '{';
        if (fmt) {
            *ptr++ = '\n';
            for (i = 0; i < depth - 1; i++)     // 缩进
                *ptr++ = '\t';
        }
        *ptr++ = '}';
        *ptr = 0;
        return out;
    }

    if (p) {
        i = p->offset;
        len = fmt ? 2 : 1;
        ptr = ensure(p, len + 1);
        if (!ptr) return NULL;
        *ptr++ = '{';
        if (fmt) *ptr++ = '\n';
        *ptr = 0;
        p->offset += len;
        child = item->child;
        depth++;

        while (child) {
            if (fmt) {
                ptr = ensure(p, depth);
                if (!ptr) return NULL;
                for (j = 0; j < depth; j++)
                    *ptr++ = '\t';
                p->offset += depth;
            }
            print_string_ptr(child->string, p);

            len = fmt ? 2 : 1;
            ptr = ensure(p, len);
            if (!ptr) return NULL;
            *ptr++ = ':';
            if (fmt) *ptr++ = '\t';
            p->offset += len;

            print_value(child, depth, fmt, p);
            p->offset = update_offset(p);

            len = (fmt ? 1 : 0) + (child->next ? 1 : 0);
            ptr = ensure(p, len + 1);
            if (child->next) *ptr++ = ',';
            if (fmt) *ptr++ = '\n';
            *ptr = 0;
            p->offset += len;
            child = child->next;
        }
        ptr = ensure(p, fmt ? (depth + 1) : 2);     // '}' '\0'
        if (!ptr) return NULL;
        if (fmt) for (i = 0; i < depth - 1; i++) *ptr++ = '\t';
        *ptr++ = '}';
        *ptr = 0;
        out = (p->buffer) + i;
    } else {
        entries = (char **) cJSON_malloc(numentries * sizeof(char *));
        if (!entries) return NULL;

        names = (char **) cJSON_malloc(numentries * sizeof(char *));
        if (!names) {
            cJSON_free(entries);
            return NULL;
        }
        memset(entries, 0, sizeof(char *) * numentries);
        memset(names, 0, sizeof(char *) * numentries);

        child = item->child;
        depth++;
        if (fmt) len += depth;

        while (child) {
            names[i] = str = print_string_ptr(child->string, 0);
            entries[i++] = ret = print_value(child, depth, fmt, 0);
            if (str && ret) len += strlen(ret) + strlen(str) + 2 + (fmt ? 2 + depth : 0);
            else fail = 1;
            child = child->next;
        }

        if (!fail) out = (char *) cJSON_malloc(len);
        if (!out) fail = 1;

        if (fail) {
            for (i = 0; i < numentries; i++) {
                if (names[i]) cJSON_free(names[i]);
                if (entries[i]) cJSON_free(entries[i]);
            }
            return NULL;
        }

        *out = '{';
        ptr = out + 1;
        if (fmt) *ptr++ = '\n';
        *ptr = 0;
        for (i = 0; i < numentries; i++) {
            if (fmt) {
                for (j = 0; j < depth; j++)
                    *ptr++ = '\t';
            }
            tmplen = strlen(names[i]);
            memcpy(ptr, names[i], tmplen);
            ptr += tmplen;
            if (fmt) *ptr++ = '\n';
            *ptr = 0;
            cJSON_free(names[i]);
            cJSON_free(entries[i]);
        }

        cJSON_free(names);
        cJSON_free(entries);
        if (fmt) {
            for (i = 0; i < depth - 1; i++) *ptr++ = '\t';
        }
        *ptr++ = '}';
        *ptr = 0;
    }
    return out;
}

int cJSON_GetArraySize(const cJSON *array) {
    cJSON *c = array->child;
    int i = 0;
    while (c) {
        ++i;
        c = c->next;
    }
    return i;
}

cJSON *cJSON_GetArrayItem(const cJSON *array, int item) {
    cJSON *c = array->child;
    while (c && item > 0) {
        --item;
        c = c->next;
    }
    return c;
}

cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string) {
    cJSON *c = object->child;
    while (c && cJSON_strcasecmp(c->string, string)) c = c->next;
    return c;
}

/* 将一个 cJSON 对象添加到数组链中 */
static void suffix_object(cJSON *prev, cJSON *item) {
    prev->next = item;
    item->prev = prev;
}

/**
 * @brief 创建一个 cJSON 项的引用
 *
 * @param item 要引用的 cJSON 项
 * @return 新创建的 cJSON 引用项  ,如果内存分配失败则返回 NULL
 */
static cJSON *create_reference(cJSON *item) {
    cJSON *ref = cJSON_New_Item();
    if (!ref) return NULL;
    memcpy(ref, item, sizeof(cJSON));
    ref->string = NULL;
    ref->type |= cJSON_IsReference;
    ref->next = ref->prev = NULL;
    return ref;
}

void cJSON_AddItemToArray(cJSON *array, cJSON *item) {
    cJSON *c = array->child;
    if (!item) return;
    if (!c) {
        array->child = item;
    } else {
        while (c && c->next) c = c->next;
        suffix_object(c, item);
    }
}

void cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item) {
    if (!item) return;
    if (item->string) cJSON_free(item->string);
    item->string = cJSON_strdup(string);
    cJSON_AddItemToArray(object, item);
}

/**
 * @brief 将 cJSON 项添加到 cJSON 对象中 ,使用常量字符串作为键名
 *
 * @param object 目标 cJSON 对象
 * @param string 新项的键名 (常量字符串)
 * @param item 要添加的 cJSON 项
 */
void cJSON_AddItemToObjectCS(cJSON *object, const char *string, cJSON *item) {
    if (!item) return;
    if (!(item->type & cJSON_StringIsConst) && item->string) cJSON_free(item->string);
    item->string = (char *) string;
    item->type |= cJSON_StringIsConst;
    cJSON_AddItemToArray(object, item);
}

void cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item) {
    cJSON_AddItemToArray(array, create_reference(item));
}

void cJSON_AddItemReferenceToObject(cJSON *object, const char *string, cJSON *item) {
    cJSON_AddItemToObject(object, string, create_reference(item));
}

cJSON *cJSON_DetachItemFromArray(cJSON *array, int which) {
    cJSON *c = array->child;
    while (c && which > 0) {
        --which;
        c = c->next;
    }
    if (!c) return NULL;
    if (c->prev) c->prev->next = c->next;
    if (c->next) c->next->prev = c->prev;
    if (c == array->child) array->child = c->next;
    c->prev = c->next = NULL;
    return c;
}