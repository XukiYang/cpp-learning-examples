#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;
/*
题目：
一个背包容量为 50kg。现有三种物品：
物品1：重10kg，价值60元
物品2：重20kg，价值100元
物品3：重30kg，价值120元
物品可以切割（比如拿半件）。问：如何装使总价值最大？

思路：
算每种物品的性价比（价值÷重量）
物品1：60÷10 = 6
物品2：100÷20 = 5
物品3：120÷30 = 4
按性价比从高到低排序：物品1 → 物品2 → 物品3
依次装入，装不下就切一部分填满背包

手动演算：
先拿物品1：重10，价值60，背包剩余40
再拿物品2：重20，价值100，背包剩余20
最后拿物品3：重30但只剩20，切 20÷30 = 2/3，价值 120 × 2/3 = 80
总价值 = 60 + 100 + 80 = 240
*/
struct Item
{
    int weight;
    int value;
    double ratio; // 性价比：价值÷重量
};

bool cmp(Item a, Item b)
{
    return a.ratio > b.ratio; // 性价比高的排前面
}

double fractionalKnapsack(int W, vector<Item> &items)
{
    // 1. 计算性价比
    for (auto &it : items)
        it.ratio = (double)it.value / it.weight;

    // 2. 按性价比排序
    sort(items.begin(), items.end(), cmp);

    double total = 0;
    // 3. 逐个装入
    for (auto &it : items)
    {
        if (W >= it.weight)
        { // 能全装下
            total += it.value;
            W -= it.weight;
        }
        else
        { // 只能切一部分，背包满了
            total += it.value * ((double)W / it.weight);
            break;
        }
    }
    return total;
}

int main()
{
    vector<Item> items = {
        {10, 60, 0}, {20, 100, 0}, {30, 120, 0}};
    cout << fractionalKnapsack(50, items); // 输出：240
}