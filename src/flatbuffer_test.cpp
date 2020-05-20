#include <fstream>
#include <iostream>

#include "quote_generated.h"  // 自动生成的头文件，注意把flatbuffers的头文件夹放在当前目录

using namespace MD::quotation;
using namespace std;

int encode_quote()
{
    // 创建builder
    flatbuffers::FlatBufferBuilder builder;
    // 先序列化非标量
    auto symbol = builder.CreateString("000025");
    double sp[] = {0.23, 0.25, 0.27};
    auto sellprice = builder.CreateVector(sp, 3);
    unsigned long sv[] = {100, 1000, 10000};
    auto sellvolume = builder.CreateVector(sv, 3);

    double bp[] = {0.2, 0.21, 0.22};
    auto buyprice = builder.CreateVector(bp, 3);
    unsigned long bv[] = {100, 1000, 10000};
    auto buyvolume = builder.CreateVector(bv, 3);

    auto time = builder.CreateString("112556");
    unsigned int bq[] = {1, 2, 3, 4};
    auto buylevelqueue = builder.CreateVector(bq, 4);
    unsigned int sq[] = {3, 4, 5, 6};
    auto selllevelqueue = builder.CreateVector(sq, 4);

    // 生成根节点
    QuoteDataBuilder quote_builder(builder);
    quote_builder.add_type(0);
    quote_builder.add_symbol(symbol);
    quote_builder.add_highprice(0.25);
    quote_builder.add_lowprice(0.102);
    quote_builder.add_lastprice(0.26);
    quote_builder.add_openprice(2.22);
    quote_builder.add_precloseprice(0.32);
    quote_builder.add_sellprice(sellprice);
    quote_builder.add_sellvolume(sellvolume);
    quote_builder.add_buyprice(buyprice);
    quote_builder.add_buyvolume(buyvolume);
    quote_builder.add_totalvolume(23132);
    quote_builder.add_time(time);
    quote_builder.add_buylevelqueue(buylevelqueue);
    quote_builder.add_selllevelqueue(selllevelqueue);
    quote_builder.add_totalamount(4654654);
    quote_builder.add_totalno(88);
    quote_builder.add_iopv(20.25);

    auto quote = quote_builder.Finish();
    builder.Finish(quote);

    // 得到序列化后的数据
    const uint8_t* buf = builder.GetBufferPointer();
    cout << "text size is: " << builder.GetSize() << endl;

    // 写入文件，注意使用二进制格式，否则可能会出错
    ofstream of;
    of.open("a.txt", ios::out | ios::binary);
    of.write((const char*)buf, builder.GetSize());
    of.close();

    return 0;
}

int decode_quote()
{
    // 从文件中读取，注意使用二进制格式，否则会出错
    ifstream ifile;
    ifile.open("a.txt", ios::in | ios::binary);
    char buf[9182];
    ifile.read(buf, sizeof(buf));
    ifile.close();

    // 直接获取数据
    uint8_t* bufp = (uint8_t*)buf;
    auto quote = GetQuoteData(bufp);
    cout << "type=" << quote->type() << endl;
    cout << "symbol: " << quote->symbol()->c_str() << endl;
    cout << "highprice: " << quote->highprice() << endl;

    for (auto i = 0; i < quote->sellprice()->Length(); i++)
    {
        cout << "sellprice[" << i << "] = " << quote->sellprice()->Get(i) << ","
             << endl;
        cout << "sellvolume[" << i << "] = " << quote->sellvolume()->Get(i)
             << "," << endl;
        cout << "buyprice[" << i << "] = " << quote->buyprice()->Get(i) << ","
             << endl;
        cout << "buyvolume[" << i << "] = " << quote->buyvolume()->Get(i) << ","
             << endl;
    }
    cout << endl;

    return 0;
}

int main()
{
    encode_quote();
    decode_quote();

    return 0;
}
