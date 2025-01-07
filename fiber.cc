#include "fiber.h"

static bool debug = false;

namespace sylar
{
    static thread_local Fiber* t_fiber = nullptr;
    static thread_local std::shared_ptr<Fiber> t_thread_fiber = nullptr;
    static thread_local Fiber* t_scheduler_fiber = nullptr;

    static std::atomic<uint64_t> s_fiber_id{0};

    static std::atomic<uint64_t> s_fiber_count{0};

    void Fiber::SetThis(Fiber *f)
    {
        t_fiber = f;
    }

    std::shared_ptr<Fiber> Fiber::GetThis()
    {
        if(t_fiber)
        {
            return t_fiber->shared_from_this();//使用shared_from_this()可以避免直接传递裸指针（this指针）给其他函数，为了正确使用shared_from_this()，类需要继承std::enable_shared_from_this，并在需要生成std::shared_ptr的地方调用shared_from_this()函数。这样，通过shared_from_this()返回的shared_ptr与原始的shared_ptr共享所有权，确保了对象的正确管理和释放。
        }
        std::shared_ptr<Fiber> main_fiber(new Fiber());
        t_thread_fiber = main_fiber;
        t_scheduler_fiber = main_fiber.get();

        assert(t_fiber == main_fiber.get());

        return t_fiber->shared_from_this();
    }

    void Fiber::SetSchedulerFiber(Fiber* f)
    {
        t_scheduler_fiber = f;
    }

    uint64_t Fiber::GetFIberId()
    {
        if(t_fiber)
        {
            return t_fiber->getId();
        }
        return (uint64_t)-1;
    }

    Fiber::Fiber()
    {
        SetThis(this);
        m_state = RUNNING;

        if(getcontext(&m_ctx))
        {
            std::cerr << "Fiber() failed\n";
            pthread_exit(NULL);
        }

        m_id = s_fiber_id++;
        s_fiber_count ++;
        if(debug) std::cout << "Fiber():main id = " << m_id << std::endl;

    }
    Fiber::Fiber(std::function<void()> cb , size_t stacksize , bool run_in_scheduler)
    {
        m_state = READY;

        m_stacksize = stacksize ? stacksize::128000;
        m_stack = malloc(m_stacksize);

        if(getcontext(&m_ctx))
        {
            std::cerr << "Fiber(std::function<void()> cb,size_t stacksize , bool run_in_scheduler is failed)" << std::endl;
            pthread_exit(NULL);
        }

        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;
        makecontext(&m_ctx,&Fiber::MainFunc,0);

        m_id = s_fiber_id++;
        s_fiber_count++;
        if(debug) std::cout << "Fiber():child id = " << m_id << std::endl;
     }

     Fiber::~fiber()
     {
        s_fiber_count--;
        if(m_stack)
        {
            free(m_stack);
        }
        if(debug) std::cout << "~FIber():id = " << m_id << std::endl;
     }

     void Fiber::reset(std::function<void()> cb)
     {
        assert(m_stack != nullptr&&m_state == TERM);

        m_state = READY;
        m_cb = cb;

        if(getcontext(&m_ctx))
        {
            std::cerr << "reset() failed";
            pthread_exit(NULL);
        }

        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;
        makecontext(&ctx,&Fiber::MainFunc,0);
     }

     void Fiber::resume()
     {
        assert(m_state = READY);

        m_state = RUNNING;

        if(m_runInScheduler)
        {
            SetThis(this);
            if(swapcontext(&(t_scheduler_fiber->m_ctx),&m_ctx))
            {
                std::cerr << "resume() to t_scheduler_fiber failed\n";
                pthread_exit(NULL);
            }
        }
        else
        {
            SetThis(this);
            if(swapcontext(&(t_thread_fiber->m_ctx),&m_ctx))
            {
                std::cerr << "resume() to t_thread_fiber failed\n";
                pthread_exit(NULL);
            }
        }
     }

     void Fiber::yield()
     {
        assert(m_state==RUNNING || m_state == TERM)
        
        if(m_state!=TERM)
        {
            m_state = READY;
        }
        if(m_runInScheduler)
        {
            SetThis(t_scheduler_fiber);
            if(swapcontext(&m_ctx,&(t_scheduler_fiber->m_ctx)))
            {
                std::cerr << "yield() to t_scheduler_fiber failed\n";
                pthread_exit(NULL);
            }
        }
        else
        {
            SetThis(t_thread_fiber.get());
            if(swapcontext(&m_ctx,&(t_thread_fiber->m_ctx)))
            {
                std::cerr << "yield() to t_thread_fiber failed\n";
                pthread_exit(NULL);
            }
        }
     }

     void Fiber::MainFunc()
     {
        std::shared_ptr<Fiber> curr = GetThis();
        assert(curr!=nullptr);

        curr->m_cb();
        curr->m_cb = nullptr;
        curr->m_state = TERM;

        auto raw_ptr = curr.get();
        curr.reset();
        raw_ptr->yield();
     }
    }