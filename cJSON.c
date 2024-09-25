#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

CJSON_PUBLIC(void) cJSON_InitHooks(cJSON_Hooks *hooks) {
    if (!hooks) {
        memset(&global_hooks, '\0', sizeof(cJSON_Hooks));
    } else {
        global_hooks = *hooks;
    }
}

static unsigned char* cJSON_strdup(const unsigned char* string, const cJSON_Hooks * const hook) {
    size_t length = 0;
    unsigned char *copy = NULL;

    if (string == NULL) {
        return NULL;
    }

    length = strlen((const char*)string) + sizeof("");
    copy = (unsigned char*)hook->allocate(length);

    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, string, length);

    return copy;
}

CJSON_PUBLIC(cJSON *) cJSON_New_Item(const cJSON_Hooks * const hooks) {
    cJSON *node = (cJSON *)hooks->allocate(sizeof(cJSON));
    if (node) {
        memset(node, '\0', sizeof(cJSON));
    }
    return node;
}

/**
 * @brief 释放 cJSON 节点， 常量字符串不释放
 * 
 * @param c cJSON 节点
 */
void cJSON_Delete(cJSON *c) {
    cJSON *next = NULL;
    while (c != NULL) {
        next = c->next;
        if (!(c->type & cJSON_IsReference) && (c->child != NULL)) {
            cJSON_Delete(c->child);
        }
        if (!(c->type & cJSON_IsReference) && (c->valuestring != NULL)) {
            global_hooks.deallocate(c->valuestring);
        }
        if (!(c->type & cJSON_StringIsConst) && (c->string != NULL)) {
            global_hooks.deallocate(c->string);
        }
        global_hooks.deallocate(c);
        c = next;
    }
}

CJSON_PUBLIC(cJSON *) cJSON_CreateObject(void) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) {
        item->type = cJSON_Object;
    }
    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateTrue(void) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) {
        item->type = cJSON_True;
    }
    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateFalse(void) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) {
        item->type = cJSON_False;
    }
    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateNull(void) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) {
        item->type = cJSON_NULL;
    }
    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateNumber(double num) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) {
        item->type = cJSON_Number;
        item->valuedouble = num;
        item->valueint = (int)num;
    }
    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateString(const char *string) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) {
        item->type = cJSON_String;
        item->valuestring = (char*)cJSON_strdup((const unsigned char*)string, &global_hooks);
        if (item->valuestring == NULL) {
            cJSON_Delete(item);
            return NULL;
        }
    }
    return item;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateArray(void) {
    cJSON *item = cJSON_New_Item(&global_hooks);
    if (item) {
        item->type = cJSON_Array;
    }
    return item;
}

static void suffix_object(cJSON *prev, cJSON *item) {
    prev->next = item;
    item->prev = prev;
}

CJSON_PUBLIC(cJSON *) cJSON_CreateStringArray(const char **strings, int count) {
    size_t i = 0;
    cJSON *n = NULL;    // next
    cJSON *p = NULL;    // prev
    cJSON *a = NULL;   // array

    if ((count < 0) || (strings == NULL)) {
        return NULL;
    }

    a = cJSON_CreateArray();

    for (i = 0; a && (i < (size_t)count); i++) {
        n = cJSON_CreateString(strings[i]);
        if (n == NULL) {
            cJSON_Delete(a);    // 创建失败，释放 a
            return NULL;
        }
        if (i == 0) {
            a->child = n;   // 第一个节点, 将 n 赋值给 a->child
        }
        else {
            suffix_object(p, n);    // 将 n 追加到 p 后面
        }
        p = n;  // p 指向 n
    }
    return a;
}

/**
 * @brief 将 item 添加到 obj 的 child 链中
 * @retval 1：成功
 */
static cJSON_bool add_item_to_list(cJSON *list, cJSON *item) {
    cJSON *child = NULL;

    if ((item == NULL) || (list == NULL)) {
        return 0;
    }

    child = list->child;       
    if (child == NULL) {
        list->child = item;    // array 没有子节点，直接将 item 赋值给 array->child
    } else {
        while (child && child->next) {  // 将 child 移动到链表的最后一个节点
            child = child->next;
        }
        suffix_object(child, item);     // 将 item 追加到 child 后面
    }
}

/**
 * @brief 将 item 以 string 为 key 添加到 object 中
 * @note item 可以是 任意cJSON类型
 * 
 * @param object 
 * @param string 
 * @param item 
 * @param hooks 
 * @param constant_key key 是否是外部定义的常量
 * @return cJSON_bool 
 * @retval 1：成功
 */
static cJSON_bool add_item_to_object(cJSON *object, const char * const string, cJSON * const item, const cJSON_Hooks * const hooks, const cJSON_bool constant_key) {
    char *new_key = NULL;
    int new_type = cJSON_String;

    if ((object == NULL) || (string == NULL) || (item == NULL)) {
        return 0;
    }

    // 如果 key 是常量，直接赋值给 new_key
    if (constant_key) {
        new_key = (char*)string;    // key 是常量
        new_type = item->type | cJSON_StringIsConst;
    } else {
        new_key = (char*)cJSON_strdup((const unsigned char*)string, hooks);
        if (new_key == NULL) {
            return 0;
        }
        new_type = item->type & ~cJSON_StringIsConst;   // key 不是常量
    }

    if (!(item->type & cJSON_StringIsConst) && (item->string != NULL)) {    // item->string 不是常量，释放 item->string
        hooks->deallocate(item->string);
    }
    
    item->string = new_key;
    item->type = new_type;

    return add_item_to_list(object, item);   // 将 item 添加到 object 中
}

