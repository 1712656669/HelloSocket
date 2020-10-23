#include <functional>

int funA(int a, int b)
{
    printf("funA\n");
    return 0;
}

int main()
{
    /*std::function<int(int, int)> call = funA;
    int n = call(1, 2);*/

    std::function<int(int, int)> call;

    int n = 5;
    //匿名函数
    call = [/*外部变量捕获列表*/n](/*参数列表*/int a, int b) -> int/*返回值类型*/
    {
        //函数体
        printf("%d\n", n + a + b);
        return 2;
    };
    int f = call(3, 1);

    return 0;
}