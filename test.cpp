#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "cJSON.h"

int main(int argc, char *argv[]) {
    char *test_s = "{\"name\":\"test\", \"age\": 18, \"sex\": \"male\", \"is_student\": true}";

    cJSON_Hooks hooks;
    hooks.malloc_fn = malloc;
    hooks.free_fn = free;

    cJSON_InitHooks(&hooks);

    cJSON *root = cJSON_Parse(test_s);
    if (root == NULL) {
        printf("Parse failed\n");
        return -1;
    }

    cJSON *name = cJSON_GetObjectItem(root, "name");
    if (name == NULL) {
        printf("Get name failed\n");
        return -1;
    }
    printf("name: %s\n", name->valuestring);

    cJSON *age = cJSON_GetObjectItem(root, "age");
    if (age == NULL) {
        printf("Get age failed\n");
        return -1;
    }
    printf("age: %d\n", age->valueint);

    cJSON *sex = cJSON_GetObjectItem(root, "sex");
    if (age == NULL) {
        printf("Get sex failed\n");
        return -1;
    }
    printf("sex: %s\n", age->valuestring);

    cJSON *is_student = cJSON_GetObjectItem(root, "is_student");
    if (is_student == NULL) {
        printf("Get is_student failed\n");
        return -1;
    }
    printf("is_student: %s\n", is_student->valueint ? "true" : "false");

    cJSON_Delete(root);
}