#include <iostream>

class Product {
public:
    virtual void use() = 0;
};

class ConcreteProductA : public Product {
public:
    void use() override {
        std::cout << "Using ConcreteProductA" << std::endl;
    }
};

class ConcreteProductB : public Product {
public:
    void use() override {
        std::cout << "Using ConcreteProductB" << std::endl;
    }
};

class Creator {
public:
    virtual Product* factoryMethod() = 0;
};

class ConcreteCreatorA : public Creator {
public:
    Product* factoryMethod() override {
        return new ConcreteProductA();
    }
};

class ConcreteCreatorB : public Creator {
public:
    Product* factoryMethod() override {
        return new ConcreteProductB();
    }
};

int main() {
    ConcreteCreatorA creatorA;
    ConcreteCreatorB creatorB;
    
    Product* productA = creatorA.factoryMethod();
    Product* productB = creatorB.factoryMethod();
    
    productA->use();
    productB->use();
    
    delete productA;
    delete productB;

    return 0;
}
