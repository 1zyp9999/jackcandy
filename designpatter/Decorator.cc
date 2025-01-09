#include <iostream>

class Component {
public:
    virtual void operation() = 0;
};

class ConcreteComponent : public Component {
public:
    void operation() override {
        std::cout << "ConcreteComponent operation" << std::endl;
    }
};

class Decorator : public Component {
protected:
    Component* component;
public:
    Decorator(Component* component) : component(component) {}
    void operation() override {
        component->operation();
    }
};

class ConcreteDecoratorA : public Decorator {
public:
    ConcreteDecoratorA(Component* component) : Decorator(component) {}
    void operation() override {
        Decorator::operation();
        std::cout << "Adding functionality A" << std::endl;
    }
};

class ConcreteDecoratorB : public Decorator {
public:
    ConcreteDecoratorB(Component* component) : Decorator(component) {}
    void operation() override {
        Decorator::operation();
        std::cout << "Adding functionality B" << std::endl;
    }
};

int main() {
    ConcreteComponent component;
    ConcreteDecoratorA decoratorA(&component);
    ConcreteDecoratorB decoratorB(&component);
    
    std::cout << "Using ConcreteDecoratorA:" << std::endl;
    decoratorA.operation();
    
    std::cout << "Using ConcreteDecoratorB:" << std::endl;
    decoratorB.operation();
    
    return 0;
}
