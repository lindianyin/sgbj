/*
 *
 * Copyright (C) 2009 jack.wgm, microcai.
 * For conditions of distribution and use, see copyright notice 
 * in (http://code.google.com/p/netsever/source/browse/trunk/COPYING)
 *
 * Author: jack.wgm
 * Email:  jack.wgm@gmail.com
 */

#ifndef WORKER_H__
#define WORKER_H__
#include <execinfo.h>
#include <sys/syscall.h>
#include "jobqueue.hpp"

extern volatile int g_print_debug_info;

namespace net 
{
    template <typename Job>
    class worker
    {
    public:
        worker(const std::string& name, jobqueue<Job>& _jobqueue, std::size_t _maxthreads = 1);
        virtual ~worker(void);

    public:
        void run();
        void stop();
        virtual bool work(Job& task) = 0;       // 派生类需要重载此虚函数,以完成工作.
        int running();
    protected:
        void workloop();                    // 工作循环.

    private:
        std::string worker_name;
        std::vector<boost::shared_ptr<boost::thread> > threads_;
        boost::mutex mutex_;
        std::size_t maxthreads_;
        volatile int _runing_loop;
        jobqueue<Job>& jobqueue_;
        volatile bool exitthread;
    }; 

    template <typename Job>
    worker<Job>::worker(const std::string& name,jobqueue<Job>& _jobqueue, std::size_t _maxthreads/* = 4*/) :
    worker_name(name),
    maxthreads_(_maxthreads),
    _runing_loop(0),
    jobqueue_(_jobqueue),
    exitthread(false)
    {
    }

    template <typename Job>
    worker<Job>::~worker(void)
    {
    }

    template <typename Job>
    void worker<Job>::run()
    {
        try
        {
            for (std::size_t i = 0; i < maxthreads_; ++i) {
                boost::shared_ptr<boost::thread> _thread(new boost::thread(
                    boost::bind(&worker::workloop, this)));
                threads_.push_back(_thread);
            }

            for (std::size_t i = 0; i < maxthreads_; ++i) {
                threads_[i]->join();
            }
        }
        catch (std::exception& e)
        {
            std::cout << "ERROR INFO:" << e.what() << std::endl;
        }
    }

    template <typename Job>
    void worker<Job>::stop()
    {
        exitthread = true;
        jobqueue_.notify_all();
    }

    template <typename Job>
    int worker<Job>::running()
    {
        return _runing_loop;
    }

    template <typename Job>
    void worker<Job>::workloop()               // 所有工作在些完成.
    {
#ifdef DEBUG_PER        
        time_t last_time = 0;
        uint64_t processed_cmd = 0;
#endif
        ++_runing_loop;
        //cout<<" ************************* "<<worker_name<<" wookloop , tid="<<syscall(SYS_gettid)<<" *************************  "<<endl;
        do 
        {
#ifdef DEBUG_PER
            //if (g_print_debug_info)
            {
                uint64_t processed = jobqueue_._processed_cmds-processed_cmd;
                time_t time_now = time(NULL);
                if (last_time != time_now && time_now % 10 == 0)
                {    
                    uint64_t inqueue = jobqueue_._total_cmds - jobqueue_._processed_cmds;
                    if (processed >= 2000 || inqueue > 100)
                    {
                        cout<<"==============="<<worker_name<<" : "<<dec<<jobqueue_._processed_cmds<<"/"<<inqueue<<"("<<processed<<"/10s)"<<endl<<flush;
                    }
                    processed_cmd = jobqueue_._processed_cmds;
                }
                last_time = time_now;
            }
#endif
            try
            {
                Job task_ = jobqueue_.getjob();
                if (work(task_))
                    continue;
                else
                    break;
            }
            catch (std::exception& e)
            {
                std::cerr << "wrok loop Exception: " << e.what() << "\n";
                void * array[25];
                int nSize = backtrace(array, 25);
                char ** symbols = backtrace_symbols(array, nSize);
                for (int i = 0; i < nSize; i++)
                {
                    cout << symbols[i] << endl;
                }
                free(symbols);
                --_runing_loop;
                return;
            }
        } while (!exitthread);
        std::cout<< "**************** "<<worker_name<<" workloop break *****************" <<endl;
        --_runing_loop;
        return;
    }

} // namespace net

#endif // WORKER_H__

