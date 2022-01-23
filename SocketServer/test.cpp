#include <iostream>
#include "DataExchange.h"
using namespace std;
int main() {
    cout << GET_TIME << endl;
    cout << sizeof(GET_TIME) << endl;
    cout << GET_INFO << endl;
    cout << sizeof(GET_INFO) << endl;
    printf("\e[H\e[2J\e[3J\e[?25l");
}