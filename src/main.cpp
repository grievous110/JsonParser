#include "JsonParser.h"
#include <iostream>
#include <random>
#include <chrono>

using namespace std;
using namespace chrono;

int main() {
    try {
        int tries = 100000;
        nanoseconds totalDuration = nanoseconds::zero();
        for (int i = 0; i < tries; i++) {
            int rn = 1 + rand() % 1000;
            string js = "{\"test\": " + rn + string("}");
            
            steady_clock::time_point start = high_resolution_clock::now();
            Json::JsonObject jsObject = Json::deserialize(js);
            steady_clock::time_point end = high_resolution_clock::now();
            totalDuration += duration_cast<nanoseconds>(end-start);
        }

        cout << "Total duration: " << static_cast<double>((totalDuration / tries).count()) / 1000000.0 << " ms" << endl;
    } catch (std::exception& e) {
        cerr << e.what() << endl;
    }
}