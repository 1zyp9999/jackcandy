#include <iostream>

class Strategy {
public:
    virtual void execute() = 0;
};

class ConcreteStrategyA : public Strategy {
public:
    void execute() override {
        std::cout << "Executing Strategy A" << std::endl;
    }
};

class ConcreteStrategyB : public Strategy {
public:
    void execute() override {
        std::cout << "Executing Strategy B" << std::endl;
    }
};

class Context {
private:
    Strategy* strategy;
public:
    Context(Strategy* strategy) : strategy(strategy) {}
    void setStrategy(Strategy* strategy) {
        this->strategy = strategy;
    }
    void executeStrategy() {
        strategy->execute();
    }
};

int main() {
    ConcreteStrategyA strategyA;
    ConcreteStrategyB strategyB;
    
    Context context(&strategyA);
    context.executeStrategy();  // Using Strategy A
    
    context.setStrategy(&strategyB);
    context.executeStrategy();  // Switching to Strategy B
    
    return 0;
}
