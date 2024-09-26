

#ifndef CJSON__H
#define CJSON__H

#ifdef __cplusplus      // C++ 中使用 C 语言的库
extern "C"
{
#endif

#if !defined(WINDOWS_) && (defined(WIN32) || defined(WIN64) || defined(_MSC_VER) || defined(_WIN32))
#define WINDOWS_
#endif

#ifdef WINDOWS_


#define CJSON_CDECL __cdecl     // 函数调用约定，__cdecl 表示 C 语言调用约定
#define CJSON_STDCALL __stdcall // 函数调用约定，__stdcall 表示标准调用约定

#if !defined(CJSON_HIDE_SYMBOLS) && !defined(CJSON_EXPORT_SYMBOLS) && !defined(CJSON_IMPORT_SYMBOLS)    // 定义导出和导入符号
#define CJSON_EXPORT_SYMBOLS
#endif

#if defined(CJSON_HIDE_SYMBOLS)   // 隐藏符号

#define CJSON_PUBLIC(type) type CJSON_STDCALL

#elif defined(CJSON_EXPORT_SYMBOLS)    // 导出符号
#define CJSON_PUBLIC(type) __declspec(dllexport) type CJSON_STDCALL     // __declspec(dllexport) 表示导出符号

#elif defined(CJSON_IMPORT_SYMBOLS)    // 导入符号
#define CJSON_PUBLIC(type) __declspec(dllimport) type CJSON_STDCALL     // __declspec(dllimport) 表示导入符号
#endif

#else // 非 Windows 平台
#define CJSON_CDECL
#define CJSON_STDCALL

#if (defined(__GNUC__) || defined(__SUNPRO_CC) || defined(__SUNPRO_C)) && defined(CJSON_API_VISIBILITY)    // GCC 编译器
#define CJSON_PUBLIC(type) __attribute__((visibility("default"))) type      // __attribute__((visibility("default"))) 表示默认可见性
#else
#define CJSON_PUBLIC(type) type
#endif
#endif

/* project version */
#define CJSON_VERSION_MAJOR 0
#define CJSON_VERSION_MINOR 0
#define CJSON_VERSION_PATCH 1

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

#define cJSON_bool int

// 为了防止堆栈溢出，限制 cJSON 在拒绝解析之前嵌套数组/对象的深度
#ifndef CJSON_NESTING_LIMIT
#define CJSON_NESTING_LIMIT 1000
#endif

//为了防止堆栈溢出，在 cJSON 拒绝解析之前，限制循环引用的长度
#ifndef CJSON_CIRCULAR_LIMIT
#define CJSON_CIRCULAR_LIMIT 10000
#endif

//以 string 返回 cJSON 的版本
CJSON_PUBLIC(const char *) cJSON_Version();

// 为 cJSON 提供 malloc, realloc 和 free 函数
void cJSON_InitHooks(cJSON_Hooks* hooks);

CJSON_PUBLIC(cJSON *) cJSON_New_Item(const cJSON_Hooks* hooks);

static unsigned char* cJSON_strdup(const unsigned char* string, const cJSON_Hooks * hook);

CJSON_PUBLIC(cJSON *) cJSON_CreateObject();

CJSON_PUBLIC(cJSON *) cJSON_CreateArray();

CJSON_PUBLIC(cJSON *) cJSON_CreateString(const char *string);

CJSON_PUBLIC(cJSON *) cJSON_CreateTrue(cJSON_bool b);

CJSON_PUBLIC(cJSON *) cJSON_CreateFalse(cJSON_bool b);

CJSON_PUBLIC(cJSON *) cJSON_CreateNumber(double num);

CJSON_PUBLIC(cJSON *) cJSON_CreateNull();

CJSON_PUBLIC(void) cJSON_Delete(cJSON *item);

CJSON_PUBLIC(cJSON *) cJSON_Parse(const char *value);

#ifdef __cplusplus
}
#endif

#endif
