#include "JsonParser.h"
#include <iostream>

using namespace std;

int main() {
    try {
        string js = "{\"test\": 1.3.2ghg}";
        Json::JsonObject jsObject = Json::deserialize(js);
        cout << jsObject["test"].toInt() << endl;
    } catch (std::exception& e) {
        cerr << e.what() << endl;
    }
}