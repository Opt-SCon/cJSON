
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

/**
 * @brief 设置 cJSON 内存分配函数。
 * @param hooks
 * @return void
 */
extern void cJSON_InitHooks(cJSON_Hooks *hooks);

/**
 * @brief 将 JSON 字符串解析为 cJSON 对象。
 * @param string：要解析的 JSON 字符串。
 * @return 返回解析后的 cJSON 对象。
 */
cJSON *cJSON_Parse(const char *string);

/**
 * @brief 将 cJSON 对象转换为 JSON 字符串（格式化）。
 * @param item：要转换的 cJSON 对象。
 * @return 返回转换后的 JSON 字符串。
 */
char *cJSON_Print(cJSON *item);

/**
 * @brief 将 cJSON 对象转换为 JSON 字符串（不格式化）。
 * @param item：要转换的 cJSON 对象。
 * @return 返回转换后的 JSON 字符串。
 */
char *cJSON_PrintUnformatted(cJSON *item);

/**
 * @brief 将 cJSON 对象转换为 JSON 字符串（缓冲区）。
 * @param item：要转换的 cJSON 对象。
 * @param prebuffer：缓冲区大小。
 * @param fmt：是否格式化。
 * @return 返回转换后的 JSON 字符串。
 */
char *cJSON_PrintBuffered(cJSON *item, int prebuffer, cJSON_bool fmt);

/**
 * @brief 释放 cJSON 对象。
 * @param c：要释放的 cJSON 对象。
 * @return void
 */
void cJSON_Delete(cJSON *c);


/**
 * @brief 获取 cJSON 数组（或对象）中的元素个数。
 * @param array：要获取元素个数的 cJSON 对象。
 * @return 返回 cJSON 数组（或对象）中的元素个数。
 */
int cJSON_GetArraySize(const cJSON *array);

/**
 * @brief 从 cJSON 数组中获取指定索引的元素。
 * @param array：要获取元素的 cJSON 数组。
 * @param index：要获取元素的索引。
 * @retval 返回指定索引的元素。
 * @retval 若检索失败，则返回 NULL。
 */
cJSON *cJSON_GetArrayItem(const cJSON *array, int index);

/**
 * @brief 从 cJSON 对象中获取指定键的元素(不区分大小写)。
 * @param object：要获取元素的 cJSON 对象。
 * @param string：要获取元素的键。
 * @retval 返回指定键的元素。
 * @retval 若检索失败，则返回 NULL。
 */
cJSON *cJSON_GetObjectItem(const cJSON *object, const char *string);

/**
 * @brief 用于分析解析失败的情况。
 * @return 返回解析失败的位置。
 * @note 当 cJSON_Parse() 返回 NULL 时定义，当 cJSON_Parse() 成功时返回 NULL。
 */
const char *cJSON_GetErrorPtr(void);


/* cJSON 各种类型的创建函数 */

cJSON *cJSON_CreateNull(void);
cJSON *cJSON_CreateTrue(void);
cJSON *cJSON_CreateFalse(void);
cJSON *cJSON_CreateBool(cJSON_bool b);
cJSON *cJSON_CreateNumber(double num);
cJSON *cJSON_CreateString(const char *string);
cJSON *cJSON_CreateArray(void);
cJSON *cJSON_CreateObject(void);

/* 创建 cJSON 数组的函数 */

cJSON *cJSON_CreateIntArray(const int *numbers, int count);
cJSON *cJSON_CreateFloatArray(const float *numbers, int count);
cJSON *cJSON_CreateDoubleArray(const double *numbers, int count);
cJSON *cJSON_CreateStringArray(const char **strings, int count);

/* cJSON 数组的操作函数 */

void cJSON_AddItemToArray(cJSON *array, cJSON *item);
void cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item);
void cJSON_AddItemToObjectCS(cJSON *object, const char *string, cJSON *item);	// 当 string 是常量时使用
void cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item);		// 将引用追加到数组中
void cJSON_AddItemReferenceToObject(cJSON *object, const char *string, cJSON *item);	// 将引用追加到对象中

/* cJSON 数组的移除/分离函数 */

cJSON *cJSON_DetachItemFromArray(cJSON *array, int which);	// 从数组中分离指定索引的元素
void cJSON_DeleteItemFromArray(cJSON *array, int which);	// 从数组中删除指定索引的元素
cJSON *cJSON_DetachItemFromObject(cJSON *object, const char *string);
void cJSON_DeleteItemFromObject(cJSON *object, const char *string);

/* cJSON 数组元素操作函数 */

void cJSON_InsertItemInArray(cJSON *array, int which, cJSON *newitem);	// 在数组中插入元素，整体右移
void cJSON_ReplaceItemInArray(cJSON *array, int which, cJSON *newitem);	// 替换数组中指定索引的元素
void cJSON_ReplaceItemInObject(cJSON *object, const char *string, cJSON *newitem);	// 替换对象中指定键的元素

/**
 * @brief 拷贝 cJSON 对象，分配新的内存。
 * @param item ：要拷贝的 cJSON 对象。
 * @param recurse ：是否递归拷贝。
 * @retval 返回拷贝后的 cJSON 对象，->next 和 ->prev 指针为 NULL。
 * @note 递归拷贝会拷贝整个 cJSON 对象。需要手动释放。
 */
cJSON *cJSON_Duplicate(cJSON *item, cJSON_bool recurse);

/**
 * @brief 解析给定的 JSON 字符串，并根据选项返回 cJSON 对象。
 * @param value：要解析的 JSON 字符串。
 * @param return_parse_end：可选参数，若非空，解析结束后将指向最后一个已解析字符的下一个位置。
 * @param require_null_terminated：如果为 cJSON_True，解析器将检查 JSON 字符串是否以空字符结尾。
 * @return 成功返回 cJSON 对象，失败返回 NULL。
 */
cJSON *cJSON_ParseWithOpts(const char *value, const char **return_parse_end, cJSON_bool require_null_terminated);

/**
 * @brief 压缩给定的 JSON 字符串，去掉所有空白字符。
 * @param json ：要压缩的 JSON 字符串。
 */
void cJSON_Minify(char *json);

/* 快速创建 cJSON 对象的宏 */

#define cJSON_AddNullToObject(object, name) cJSON_AddItemToObject(object, name, cJSON_CreateNull())
#define cJSON_AddTrueToObject(object, name) cJSON_AddItemToObject(object, name, cJSON_CreateTrue())
#define cJSON_AddFalseToObject(object, name) cJSON_AddItemToObject(object, name, cJSON_CreateFalse())
#define cJSON_AddBoolToObject(object, name, b) cJSON_AddItemToObject(object, name, cJSON_CreateBool(b))
#define cJSON_AddNumberToObject(object, name, n) cJSON_AddItemToObject(object, name, cJSON_CreateNumber(n))
#define cJSON_AddStringToObject(object, name, s) cJSON_AddItemToObject(object, name, cJSON_CreateString(s))

/* 当分配整数值时，也需要将其传递到双精度值 */
#define cJSON_SetIntValue(object, val) ((object) ? (object)->valueint = (object)->valuedouble = (val) : (val))
#define cJSON_SetNumberValue(object, val) ((object) ? (object)->valueint = (object)->valuedouble = (val) : (val))


#ifdef __cplusplus
}
#endif

#endif