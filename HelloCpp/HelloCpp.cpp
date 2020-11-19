#include <functional>

int funA(int a, int b)
{
    printf("funA\n");
    return 0;
}

int main()
{
    //std::function 是一个可调用对象包装器，是一个类模板，可以容纳除了类成员函数指针之外的所有可调用对象
    //它可以用统一的方式处理函数、函数对象、函数指针，并允许保存和延迟它们的执行
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