#include <iostream>
#include <vector>
using namespace std;
/*
题目：
同样容量50kg，三种物品重量与价值同上。但这次物品不可切割，要么全拿，要么不拿。问：最大价值？
思路：用 DP 表逐个决策。

DP递推式：
dp[i][w] = max(dp[i-1][w], dp[i-1][w-wt[i]] + val[i])
i：考虑前i个物品
w：背包剩余容量
dp[i][w]：前i个物品、容量w下的最大价值

填表过程：
i/w	0	10	20	30	40	50
0（无物品）	0	0	0	0	0	0
1（物品1:10,60）	0	60	60	60	60	60
2（物品2:20,100）	0	60	100	160	160	160
3（物品3:30,120）	0	60	100	160	160	220

关键几步拆解：
dp[2][30]：物品1(10)+物品2(20)=30，价值60+100=160   
dp[3][50]：看dp[2][50]=160，再看dp[2][50-30]+120=dp[2][20]+120=100+120=220，取max=220
*/
int knapsack01(int W, vector<int> &wt, vector<int> &val)
{
    int n = wt.size();
    vector<vector<int>> dp(n + 1, vector<int>(W + 1, 0));

    for (int i = 1; i <= n; i++)
    {
        for (int w = 0; w <= W; w++)
        {
            if (wt[i - 1] > w)
            { // 装不下当前物品
                dp[i][w] = dp[i - 1][w];
            }
            else
            { // 装或不装取最大值
                dp[i][w] = max(dp[i - 1][w], dp[i - 1][w - wt[i - 1]] + val[i - 1]);
            }
        }
    }
    return dp[n][W]; // 右下角即最优解
}

int main()
{
    vector<int> wt = {10, 20, 30};    // 各物品重量
    vector<int> val = {60, 100, 120}; // 各物品价值
    int W = 50;
    cout << "最大价值：" << knapsack01(W, wt, val); // 220
    return 0;
}