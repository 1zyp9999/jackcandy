#include <iostream>

class Singleton {
private:
    static Singleton* instance;
    Singleton() {}  // 私有构造函数

public:
    static Singleton* getInstance() {
        if (instance == nullptr) {
            instance = new Singleton();
        }
        return instance;
    }

    void showMessage() {
        std::cout << "Singleton instance is working!" << std::endl;
    }
};

Singleton* Singleton::instance = nullptr;

int main() {
    Singleton* s1 = Singleton::getInstance();
    Singleton* s2 = Singleton::getInstance();
    
    s1->showMessage();
    if (s1 == s2) {
        std::cout << "Both instances are the same." << std::endl;
    }
    
    return 0;
}
