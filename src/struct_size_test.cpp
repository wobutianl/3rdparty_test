/*
 * @Description: 
 * @Version: 2.0
 * @Autor: 
 * @Date: 2020-04-15 10:36:59
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2020-04-15 14:03:31
 */


#include <iostream>
#include<vector>

using namespace std;

using ID = long;

int main(){
    cout<<"long size :"<<sizeof(long)<<endl;
    cout<<"int64_t size: "<<sizeof(int64_t)<<endl;

    ID a = 10;
    int64_t b = 5;

    a = reinterpret_cast<ID>(b);
    cout<<"ID"<<a<<endl;

    return 0;
}

