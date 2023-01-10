#include <mutex>
#include <condition_variable>
#include <vector>

template <typename T>
class ReturnObjectDelivery;

template <typename T>
class ReturnObject;

template <typename T>
class ReturnObjectDelivery
{
private:
public:
    std::vector<ReturnObject<T>*> pROs;
    
public:

    void set_value(T& value) 
    {
        for (ReturnObject<T>* pRO : pROs)
        {
            if (pRO == nullptr) continue;
            pRO->returnValue = value;
            pRO->returned = true;
            pRO->cvReturnWait.notify_one();
        }
    }
};


template <typename T>
class ReturnObject 
{
private:
    std::mutex mtx;
public:
    T returnValue;
    bool connected = false;
    bool returned = false;
    std::condition_variable cvReturnWait;
public:
    ReturnObject() = default;
    ReturnObject(const ReturnObject&) = delete;


    void connectROD(ReturnObjectDelivery<T>* _pROD) 
    { 
        if (_pROD == nullptr) return;
        this->connected = true;
        _pROD->pROs.push_back(this);
    }

    T get()
    {
        if (!connected) return T{};
        std::unique_lock<std::mutex> ul(this->mtx);
        if (!returned) cvReturnWait.wait(ul, [this]{ return this->returned; }); // if not yet returned, wait until returned
        return returnValue;
    }
};