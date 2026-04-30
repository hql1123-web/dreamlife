#include <iostream>
#include <string>
int main() {
    std::string userInput;
    std::cout << "请输入内容：";
    std::getline(std::cin, userInput);
    std::cout << "你输入的是：" << userInput << std::endl;
    return 0;
}
