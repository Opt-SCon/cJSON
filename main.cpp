#include <stdio.h>
#include <stdlib.h>

#include "cJSON.h"

int main(int argc, char *argv[]) {
	const char *json = "{\"name\":\"Jack\",\"age\":27}";
	cJSON *root = cJSON_Parse(json);
	if (json == NULL) {
		printf("Error before: [%s]\n", cJSON_GetErrorPtr());
	} else {
		const char *name = cJSON_GetObjectItem(root, "name")->valuestring;
		int age = cJSON_GetObjectItem(root, "age")->valueint;
		printf("name: %s, age: %d\n", name, age);
	}
}