#include <iostream>
#include <vector>
#include <algorithm>
class Observer {
public:
    virtual void update(int state) = 0;
};

class ConcreteObserver : public Observer {
private:
    int state;
public:
    void update(int state) override {
        this->state = state;
        std::cout << "Observer updated with state: " << state << std::endl;
    }
};

class Subject {
private:
    std::vector<Observer*> observers;
    int state;
public:
    void addObserver(Observer* observer) {
        observers.push_back(observer);
    }
    void removeObserver(Observer* observer) {
        observers.erase(std::remove(observers.begin(), observers.end(), observer), observers.end());
    }
    void setState(int state) {
        this->state = state;
        notifyObservers();
    }
    void notifyObservers() {
        for (auto observer : observers) {
            observer->update(state);
        }
    }
};

int main() {
    Subject subject;
    ConcreteObserver observer1, observer2;
    
    subject.addObserver(&observer1);
    subject.addObserver(&observer2);
    
    std::cout << "Setting state to 10" << std::endl;
    subject.setState(10);  // All observers will be notified

    std::cout << "Setting state to 20" << std::endl;
    subject.setState(20);  // All observers will be notified

    return 0;
}
