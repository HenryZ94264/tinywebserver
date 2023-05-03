#ifndef HEAP_TIMER
#define HEAP_TIMER

#include "base_timer.h"

// const int BUFFER_SIZE = 64;

class heap_timer : public base_timer
{
public:
    heap_timer(int delay): id(-1)
    {
        expire = time(nullptr) + delay;
    }
    heap_timer(int delay, int id): id(id)
    {
        expire = time(nullptr) + delay;
    }
    virtual ~heap_timer() final {};
public:
    int id;
};

// 时间堆类
class time_heap : public timer_structure
{
public:
    time_heap() throw(std::exception) : cur_size(0)
    {
        array = new heap_timer* [capacity];
        if(!array)
        {
            throw std::exception();
        }
        for(int i = 0; i < capacity; ++i)
        {
            array[i] = nullptr;
        }
    }

    // 构造函数一，初始化一个大小为cap的空堆
    time_heap(int cap) throw (std::exception) : capacity(cap), cur_size(0)
    {
        array = new heap_timer* [capacity];     // 创建堆数组
        if(!array)
        {
            throw std::exception();
        }
        for(int i = 0; i < capacity; ++i)
        {
            array[i] = nullptr;
        }
    }
    
    // 构造函数之二，用已有数组来初始化堆
    time_heap(heap_timer** init_array, int size, int capacity) throw (std::exception) : capacity(capacity), cur_size(size)
    {
        if(capacity < size)
        {
            throw std::exception();
        }
        array = new heap_timer* [capacity];
        if(!array)
        {
            throw std::exception();
        }
        for(int i = 0; i < capacity; ++i)
        {
            array[i] = nullptr;
        }
        // 初始化堆数组
        if(size != 0)
        {
            for(int i = 0; i < size; ++i)
            {
                array[i] = init_array[i];
            }
            for(int i = (cur_size - 1) / 2; i>=0; --i)
            {

                percolate_down(i);
            }
        }
    }

    void add_timer(base_timer* timer) throw (std::exception) override
    {
        heap_timer* temp_timer = dynamic_cast<heap_timer*>(timer);
        if(!temp_timer)
        {
            return;
        }
        if(cur_size >= capacity)
        {
            resize();
        }
        int hole = cur_size++;
        int parent = 0;
        for(; hole > 0; hole = parent)
        {
            parent = (hole-1)/2;
            if(array[parent]->expire <= temp_timer->expire)
            {
                break;
            }
            else
            {
                array[hole] = array[parent];
            }
        }
        temp_timer->id = hole;
        array[hole] = temp_timer;
        
    }

    void del_timer(base_timer* timer) override
    {
        heap_timer* temp_timer = dynamic_cast<heap_timer*>(timer);
        if(!temp_timer)
        {
            return;
        }
        timer->cb_func = NULL;  // 仅仅将回调函数设置为空
    }

    heap_timer* top() const
    {
        if(cur_size == 0) return nullptr;
        return array[0];
    }

    void pop_timer()
    {
        if(cur_size == 0) return;
        if(array[0])
        {
            delete array[0];
            array[0] = array[--cur_size];
            percolate_down(0);
        }
    }

    void tick()
    {
        heap_timer* tmp = array[0];
        time_t cur = time(NULL);
        while(cur_size!=0)
        {
            if(!tmp) break;
            if(tmp->expire > cur)
            {
                break;
            }
            if(array[0]->cb_func)
            {
                array[0]->cb_func(array[0]->user_data);
            }
            pop_timer();
            tmp=array[0];
        }
    }

    void adjust_timer(base_timer* timer) override
    {
        heap_timer* tmp = dynamic_cast<heap_timer*>(timer);
        if(!tmp) return;
        percolate_down(tmp->id);
    }

private:
    void percolate_down(int hole)
    {
        heap_timer* temp = array[hole];
        int child = 0;
        for(; ((hole*2+1) <= (cur_size-1)) ; hole=child )
        {
            child = hole * 2 + 1;
            // 找出自己、左子、右子中最大的
            if((child < (cur_size-1)) && (array[child+1]->expire < array[child]->expire))
            {
                ++child;
            }
            if(array[child]->expire < temp->expire)
            {
                array[hole] = array[child];
            }
            else
            {
                break;
            }
        }
        temp->id = hole;
        array[hole] = temp;
    }

    void resize() throw(std::exception)
    {
        heap_timer** temp = new heap_timer* [2*capacity];
        for(int i = 0; i < 2*capacity; ++i)
        {
            temp[i] = NULL;
        }
        if(!temp)
        {
            throw std::exception();
        }
        capacity = 2 * capacity;
        for(int i = 0 ; i < cur_size; ++i)
        {
            temp[i] = array[i];
        }
        delete[] array;
        array = temp;
    }

private:
    heap_timer** array;     // 堆数组
    int capacity = 100;   // 堆数组的容量
    int cur_size;   // 堆数组当前包含元素的个数
};



#endif