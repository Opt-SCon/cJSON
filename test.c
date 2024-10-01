#include <stdio.h>
#include "cJSON.h"

int main() {
    // 创建根对象
    cJSON *root = cJSON_CreateObject();

    // 添加一个字符串
    cJSON_AddItemToObject(root, "name", cJSON_CreateString("John Doe"));

    // 添加一个数字
    cJSON_AddItemToObject(root, "age", cJSON_CreateNumber(30));

    // 添加一个布尔值
    cJSON_AddItemToObject(root, "is_student", cJSON_CreateBool(0)); // 0 表示 false

    // 添加一个数组
    cJSON *hobbies = cJSON_CreateArray();
    cJSON_AddItemToArray(hobbies, cJSON_CreateString("reading"));
    cJSON_AddItemToArray(hobbies, cJSON_CreateString("swimming"));
    cJSON_AddItemToArray(hobbies, cJSON_CreateString("coding"));
    cJSON_AddItemToObject(root, "hobbies", hobbies);

    // 添加一个嵌套对象
    cJSON *address = cJSON_CreateArray();
    cJSON *add1 = cJSON_CreateObject();
    cJSON_AddItemToObject(add1, "street", cJSON_CreateString("123 Main St"));
    cJSON_AddItemToObject(add1, "city", cJSON_CreateString("Anytown"));
    cJSON_AddItemToObject(add1, "country", cJSON_CreateString("USA"));

    cJSON *add2 = cJSON_CreateObject();
    cJSON_AddItemToObject(add2, "street", cJSON_CreateString("456 Park Ave"));
    cJSON_AddItemToObject(add2, "city", cJSON_CreateString("Othertown"));
    cJSON_AddItemToObject(add2, "country", cJSON_CreateString("USA"));

    cJSON_AddItemToArray(address, add1);
    cJSON_AddItemToArray(address, add2);
    cJSON_AddItemToObject(root, "address", address);

    // 将 JSON 打印为字符串
    char *json_str = cJSON_Print(root);
    printf("%s\n", json_str);

    // 清理
    cJSON_Delete(root);

    return 0;
}