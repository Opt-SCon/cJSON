#include <bits/stdc++.h>
#include "cJSON.hpp"
using namespace std;

int main() {

    fstream file = fstream("../.vscode/settings.json", ios::in);

    if (!file.is_open()) {
        cout << "Open file failed." << endl;
        return -1;
    }

    string json_str;
    string line;
    while (getline(file, line)) {
        json_str += line;
    }

    cout << json_str << endl;

    cJSON *root = cJSON_Parse(json_str.c_str());
    if (root == nullptr) {
        cout << "Before cJSON_Parse: " << cJSON_GetErrorPtr() << endl;
        return -1;
    }

    cout << "After cJSON_Parse: " << endl << cJSON_Print(root) << endl;

    return 0;
}