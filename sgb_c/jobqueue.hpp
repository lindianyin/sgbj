/*
 *
 * Copyright (C) 2009 jack.wgm, microcai.
 * For conditions of distribution and use, see copyright notice 
 * in (http://code.google.com/p/netsever/source/browse/trunk/COPYING)
 *
 * Author: jack.wgm
 * Email:  jack.wgm@gmail.com
 */

#ifndef JOBQUEUE_H__
#define JOBQUEUE_H__
#include "inc.h"

namespace net 
{

    template <typename Job>
    class jobqueue
    {
    public:
        jobqueue(void);
        virtual ~jobqueue(void);

        void submitjob(const Job& x);
        Job getjob();
        void notify_all();
        size_t jobcounts();

    private:
        volatile bool exitwait_;
        std::list<Job> list_;
        boost::mutex mutex_;
        boost::condition worktobedone_;
    public:
        volatile uint64_t _total_cmds;
        volatile uint64_t _processed_cmds;
    };

    template <typename Job>
        jobqueue<Job>::jobqueue(void)
        : exitwait_(false),        
        _total_cmds(0),
        _processed_cmds(0)
    {}

    template <typename Job>
        jobqueue<Job>::~jobqueue(void)
    {}

    template <typename Job>
    void jobqueue<Job>::submitjob(const Job& x)
    {
        if (!exitwait_)
        {
            boost::mutex::scoped_lock lock(mutex_);
            list_.push_back(x);
            ++_total_cmds;
            worktobedone_.notify_all();
        }
    }

    template <typename Job>
    void jobqueue<Job>::notify_all()
    {
        exitwait_ = true;
        worktobedone_.notify_all();
    }

    template <typename Job>
    Job jobqueue<Job>::getjob()
    {
        boost::mutex::scoped_lock lock(mutex_);
        while (list_.size()==0 && !exitwait_)
            worktobedone_.wait(lock);
        Job tmp;
        if (exitwait_)
            return tmp;
        tmp = list_.front();
        list_.pop_front();
        ++_processed_cmds;
        return tmp;
    }

    template <typename Job>
    size_t jobqueue<Job>::jobcounts()
    {
        boost::mutex::scoped_lock lock(mutex_);
        return list_.size();
    }
} // namespace net

#endif // JOBQUEUE_H__

