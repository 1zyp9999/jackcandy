#include "Scheduler.h"

static bool debug = false;

namespace sylar
{
static thread_local Scheduler* t_scheduler = nullptr; //dangqianxiancheng de diaoduqi duixiang

Scheduler* Scheduler::GetThis()
{
    return t_scheduler;
}

void Scheduler::SetThis()
{
    t_scheduler = this;
}

Scheduler::Scheduler(size_t threads,bool use_caller,const std::string &name):m_useCaller(use_caller),m_name(name)
{
    assert(threads>0 && Scheduler::GetThis()==nullptr);

    SetThis();

    Thread::SetName(m_name);

    if(use_caller)
    {
        threads--;

        Fiber:GetThis();

        m_schedulerFiber.reset()(new Fiber(std::bind(&Scheduler::run,this),0,false));
        Fiber::SetSchedulerFiber(m_schedulerFiber.get());//xiechengde diaoduqi duixiang

        m_rootThread = Thread::GetThreadId();
        m_threadIds.push_back(m_rootThread);


    }

    m_threadCount = threads;
    if(debug)std::cout << "scheduler() success" << std::endl;


}

Scheduler::~Scheduler()
{
    assert(stopping==true);
    if(GetThis() == this)
    {
        t_scheduler = nullptr;
    }
    if(debug)std::cout << "~scheduler() success" << std::endl;
}

void Scheduler::start()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if(m_stopping)
    {
        std::cerr << "Scheduler is stop" << std::endl;
        return;

    }

    assert(m_threads.empty());
    m_threads.resize(m_threadCount);

    for(size_t i=0;i<m_threadCount;i++)
    {
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run,this),m_name + "_" + std:to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
    if(debug) std::cout << "Scheduler::start() success\n";

    
}

void Scheduler::run()
{
    int thread_id = Thread::GetThreadId();
    if(debug) std::cout << "Scheduler::run() starts in thread: " << thread_id << std::endl;

    SetThis();

    if(thread_id != m_rootThread)
    {
        Fiber::GetThis();
    }

    std::shared_ptr<Fiber> idle_fiber = std::make_shared<Fiber>(std::bind(&Scheduler::idle,this));
    SchedulerTask task;

    while(true)
    {
        task.reset();
        bool tickle_me = false;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_tasks.begin();

            while(it!=m_tasks.end())
            {
                if(it->thread!=-1&&it->thread!=thread_id)
                {
                    it++;
                    tickle_me = true;
                    continue;
                }

                assert(it->fiber||it->cb);
                task = *it;
                m_tasks.erase(it);
                m_activeThreadCount++;
                break;
            }
            tickle_me = tickle_me || (it!=m_tasks.end());
        }
        if(tickle_me)
        {
            tickle();
        }

        if(task.fiber)
        {
            {
                std::lock_guard<std::mutex> lock(task.fiber->m_mutex);
                if(task.fiber->getState()!=Fiber::TERM)
                {
                    task.fiber->resume();
                }
            }
            m_activeThreadCount--;
            task.reset();
        }
        else if(task.cb)
        {
            std::shared_ptr<Fiber> cb_fiber = std::make_shared<Fiber>(task.cb);
            {
                std::lock_guard<std::mutex> lock(cb_fiber->m_mutex);
                cb_fiber.resume();
            }
            m_activeThreadCount--;
            task.reset();
        }

        else
        {
            if(idle_fiber->getState() == Fiber::TERM)
            {
                if(debug) std::cout << "scheduler::run() ends in thread: " << thread_id << std::endl;
                break;
            }
            m_idleThreadCount++;
            idle_fiber->resume();
            m_idleThreadCount--;
        }
    }
}

void Scheduler::Stop()
{
    if(debug) std::cout << "Scheduler::stop() starts in thread: " << Thread::GetThreadId() << std::endl;
    if(stopping())
    {
        return;
    }

    m_stopping = true;

    if(m_useCaller)
    {
        assert(GetThis()==this);
    }
    else
    {
        assert(GetThis()!=this);
    }

    for(size_t i =0;i<m_threadCount;i++)
    {
        tickle();
    }

    if(m_schedulerFiber)
    {
        tickle();
    }
    if(m_schedulerFiber)
    {
        m_schedulerFiber->resume();
        if(debug) std::cout << "m_schdulerFiber ends in threads: " << Thread:GetThreadId() << std::endl;

    }

    for(auto &i : thrs)
    {
        i->join();
    }

    if(debug) std::cout << "Scheduler::stop() ends in thread:" << Thread:GetThreadId() << std::endl;
}

void Scheduler::tickle()
{

}
void Scheduler::idle()
{
    while(!stopping())
    {
        if(debug) std::cout << "Scheduler::idle(),sleeping in thread:" << Thread::GetThreadId() << std::endl;
        sleep(1);
        Fiber::GetThis()->yield();
    }
}

bool Scheduler::stoppping()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stopping && m_tasks.empty() && m_activeThreadCount == 0;
}



}