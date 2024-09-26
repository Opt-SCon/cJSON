
#ifndef CJSON__H
#define CJSON__H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h> // size_t

/* cJSON Types: */
#define cJSON_Invalid (0)	  // 非法类型
#define cJSON_False (1 << 0)  // 假
#define cJSON_True (1 << 1)	  // 真
#define cJSON_NULL (1 << 2)	  // 空
#define cJSON_Number (1 << 3) // 数字
#define cJSON_String (1 << 4) // 字符串
#define cJSON_Array (1 << 5)  // 数组
#define cJSON_Object (1 << 6) // 对象
#define cJSON_Raw (1 << 7)	  // json 原始数据

#define cJSON_IsReference 256	// 是否引用外部数据
#define cJSON_StringIsConst 512 // 字符串是否是常量

#define cJSON_bool int

/* cJSON structure */
typedef struct cJSON {
	// 下一个节点、上一个节点
	struct cJSON *next, *prev;

	// 子节点
	struct cJSON *child;

	// 类型，取值为
	// cJSON_Invalid、cJSON_False、cJSON_True、cJSON_NULL、cJSON_Number、cJSON_String、cJSON_Array、cJSON_Object、cJSON_Raw
	int type;

	// 字符串，类型为 cJSON_String、cJSON_Raw
	char *valuestring;

	// 数字，存放整数，类型为 cJSON_Number
	int valueint;

	// 数字，存放浮点数，类型为 cJSON_Number
	double valuedouble;

	// Key 键值
	char *string;
} cJSON;

typedef struct cJSON_Hooks {
	// 内存分配函数
	void *(*malloc_fn)(size_t size);
	void (*free_fn)(void *pointer);
} cJSON_Hooks;

/* 提供 malloc，realloc 和 free 函数的钩子。 */
extern void cJSON_InitHooks(cJSON_Hooks *hooks);

/* 提供一个 JSON 块，这将返回一个可以查询的 cJSON 对象。完成后调用 cJSON_Delete 。 */
cJSON *cJSON_Parse(const char *value);

/* 从文件中读取 JSON 数据并解析。 */
cJSON *cJSON_ParseWithOpts(const char *value, const char **return_parse_end, cJSON_bool require_null_terminated);

/* 将 cJSON 对象转换为 JSON 字符串。 */
char *cJSON_Print(cJSON *item);

/* 将 cJSON 对象转换为 JSON 字符串，但不进行格式化。 */
char *cJSON_PrintUnformatted(cJSON *item);

/* 释放 cJSON 对象占用的内存。 */
void cJSON_Delete(cJSON *c);

/* 从 cJSON 对象中获取一个子对象。 */
cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string);


#ifdef __cplusplus
}
#endif

#endif