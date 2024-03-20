// Вопрос 2.
/*
* На языке С++, написать минимум 2 класса реализации циклического буфера.
* Объяснить плюсы и минусы каждой реализации.
*/

// Ответ.
/*
* Циклический буфер это структура данных фиксированного размера, использующая
* способ организации FIFO.
*
* Реализуем классы CircularBufDurable и CircularBufTransitory.
*
* Класс CircularBufDurable не позволяет сделать запись если в нём нет
* свободного места. Место считается свободным, если на нём нет маркера чтения.
* Маркер записи не может перешагнуть через маркер чтения.
* Недостаток такого решения в том, что сообщения, для которых нет места, теряются.
*
* Класс CircularBufTransitory всегда позволяет сделать запись, затирая самые 
* старые записи.
*/

#include <iostream>
#include <chrono>
#include <future>
#include <mutex>

//
//----- Declarations block -----//
//
std::mutex g_mu {};

struct Data                                                                     // Используем структуру как макет пакета данных.
{
    size_t dataPart {};
};

class CircularBufDurable final                                                  // Возможность наследования убрана.
{
    size_t     m_bufCapacity     {};
    Data*      m_bufDynamicArray {nullptr};
    size_t     m_bufWritePos     {};
    size_t     m_bufReadPos      {};
    std::mutex m_mu              {};

public:

    CircularBufDurable(size_t bufCapacity);
    CircularBufDurable(CircularBufDurable const &) = delete;                    // Возможность копирования убрана.
    CircularBufDurable operator=(CircularBufDurable const &) = delete;
    ~CircularBufDurable();

    Data read();
    bool write(Data const &data);
};

class CircularBufTransitory final                                               // Возможность наследования убрана.
{
    size_t     m_bufCapacity     {};
    Data*      m_bufDynamicArray {nullptr};
    size_t     m_bufWritePos     {};
    size_t     m_bufReadPos      {};
    bool       m_endFlag         {};
    std::mutex m_mu              {};

public:

    CircularBufTransitory(size_t bufCapacity);
    CircularBufTransitory(CircularBufTransitory const &) = delete;              // Возможность копирования убрана.
    CircularBufTransitory operator=(CircularBufTransitory const &) = delete;
    ~CircularBufTransitory();

    Data read();
    bool write(Data const &data);
};

template <typename Class>                                                       // Эта и далее до завершения блока – вспомогательные функции для демонстрации.
void writeData(Class &bufferObject);

template <typename Class>
void readData(Class &bufferObject);

template <typename Class>
void perfromProcessOnBuffer(Class &bufferObject, 
                            int tickInreval_s,
                            void (*process)(Class &bufferObject),
                            size_t executionDuration);

//
//----- Main program block -----//
//
int main()
{
    {
        std::cout << "Demonstration for CircularBufDurable class\n";
        CircularBufDurable buffer {3};
    
        auto performWriting {std::async(std::launch::async, 
                             perfromProcessOnBuffer<CircularBufDurable>, 
                             std::ref(buffer), 
                             1, 
                             writeData<CircularBufDurable>, 
                             20)};
        
        auto performReading {std::async(std::launch::async, 
                             perfromProcessOnBuffer<CircularBufDurable>, 
                             std::ref(buffer), 
                             3, 
                             readData<CircularBufDurable>, 
                             10)}; 
    }
    
    std::cout << '\n';
    
    {
        std::cout << "Demonstration for CircularBufTransitory class\n";
        CircularBufTransitory buffer {3};

        auto performWriting {std::async(std::launch::async, 
                             perfromProcessOnBuffer<CircularBufTransitory>, 
                             std::ref(buffer), 
                             1, 
                             writeData<CircularBufTransitory>, 
                             20)};
        
        auto performReading {std::async(std::launch::async, 
                             perfromProcessOnBuffer<CircularBufTransitory>, 
                             std::ref(buffer), 
                             2, 
                             readData<CircularBufTransitory>, 
                             10)};
    }
}

//
//----- Definitions block -----//
//
CircularBufDurable::CircularBufDurable(size_t bufCapacity)
    : m_bufCapacity     {bufCapacity + 1}                                       // +1 для места занимаемого маркером чтения.
    , m_bufDynamicArray {new Data[m_bufCapacity ]{}}
    {}

CircularBufDurable::~CircularBufDurable() { delete [] m_bufDynamicArray; }

Data CircularBufDurable::read()
{
    if (m_bufReadPos != m_bufWritePos)                                          // Если маркер чтения догнал маркер записи, данные закончились.
    {
        m_bufReadPos = (m_bufReadPos + 1) % m_bufCapacity;
        m_mu.lock();
        Data dataToRead {m_bufDynamicArray[m_bufReadPos].dataPart};
        m_bufDynamicArray[m_bufReadPos].dataPart = 0;
        m_mu.unlock();
        
        return dataToRead;
    }
    
    return m_bufDynamicArray[m_bufReadPos];                                     // Возвращаемый 0 – признак отсутствия данных.
}

bool CircularBufDurable::write(Data const &data)
{
    if (((m_bufWritePos + 1) % m_bufCapacity) != m_bufReadPos)                  // Если впереди маркер чтения, свободное место закончилось.
    {
        m_bufWritePos = (m_bufWritePos + 1) % m_bufCapacity;
        m_mu.lock();
        m_bufDynamicArray[m_bufWritePos].dataPart = data.dataPart;
        m_mu.unlock();
        
        return true;
    }
    
    return false;
}

CircularBufTransitory::CircularBufTransitory(size_t bufCapacity)
    : m_bufCapacity     {bufCapacity}
    , m_bufDynamicArray {new Data[m_bufCapacity ]{}}
    {}

CircularBufTransitory::~CircularBufTransitory() { delete [] m_bufDynamicArray; }

Data CircularBufTransitory::read()
{   
    if (m_endFlag)                                                              // Если флаг поднят, данных дальше нет.
    {
        Data noData {};
        return noData;                                                          // Возвращаемый 0 – признак отсутствия данных.
    }

    m_bufReadPos = (m_bufReadPos + 1) % m_bufCapacity;
    m_endFlag = m_bufReadPos == m_bufWritePos;                                  // Если достигли маркера записи, данные закончились.

    return m_bufDynamicArray[m_bufReadPos];
}

bool CircularBufTransitory::write(Data const &data)
{
    m_bufWritePos = (m_bufWritePos + 1) % m_bufCapacity;
    m_endFlag = false;                                                          // Запись добавит данные, значит флаг надо опустить.
    if (m_bufWritePos == ((m_bufReadPos + 1) % m_bufCapacity))                  // Если запись осуществится туда, где должно выполниться следующее чтение, 
        m_bufReadPos = (m_bufReadPos + 1) % m_bufCapacity;                      // смещаем маркер записи на шаг вперёд – к следующей самой старой записи.
    
    m_mu.lock();
    m_bufDynamicArray[m_bufWritePos].dataPart = data.dataPart;
    m_mu.unlock();
    
    return true;
}

//----- Определения вспомогательных функций -----//

template <typename Class>
void writeData(Class &bufferObject)
{
    static size_t data {};
    Data dataPortion {++data};
    g_mu.lock();
    std::cout << "Writing state: " << bufferObject.write(dataPortion) << '\n';
    g_mu.unlock();
}

template <typename Class>
void readData(Class &bufferObject)
{
    g_mu.lock();
    std::cout << "Read data: " << bufferObject.read().dataPart << '\n';
    g_mu.unlock();
}

template <typename Class>
void perfromProcessOnBuffer(Class &bufferObject, 
                            int tickInreval_s,
                            void (*process)(Class &bufferObject),
                            size_t executionDuration)
{    
    typedef std::chrono::steady_clock clock;
    typedef std::chrono::seconds s;
        
    std::chrono::time_point<clock> prevTick {clock::now()};

    auto tick {[&tickInreval_s](std::chrono::time_point<clock> const &prevTick) -> bool
        {
            std::chrono::time_point<clock> newTick {clock::now()};
            
            return 
            std::chrono::duration_cast<s>(newTick - prevTick).count() == tickInreval_s;
        }
    };

    for (size_t i {}; i < executionDuration;)
    {
        if (tick(prevTick))
        {
            prevTick = clock::now();
            ++i;
            process(bufferObject);
        }
    }
}