#ifndef __THREADPOOL_HPP__
#define __THREADPOOL_HPP__

#include <iostream>
#include <queue>
#include <pthread.h>

typedef void (*handler_t)(int);
class Task{
  private:
    int sock;
    handler_t handler;
  public:
    Task(int sock_,handler_t handler_):sock(sock_),handler(handler_)
  {}
    void Run()
    {
      handler(sock);
    }
    ~Task()
    {}
};

class ThreadPool{
  private:
    int num;
    std::queue<Task> task_queue;
    int idle_num;

    pthread_mutex_t lock;
    pthread_cond_t cond;

  public:
    ThreadPool(int num_):num(num_),idle_num(0)
  {
    pthread_mutex_init(&lock,NULL);
    pthread_cond_init(&cond,NULL);
  }

    void InitThreadPool()
    {
      pthread_t tid;
      for(auto i=0;i<num;i++)
      {
        pthread_create(&tid,NULL,ThreadRoutine,(void*)this);
      }
    }
    bool IsTaskQueueEmpty()
    {
      return task_queue.size() == 0?true:false;
    }
    void LockQueue()
    {
      pthread_mutex_lock(&lock);
    }
    void UnLockQueue()
    {
      pthread_mutex_unlock(&lock);
    }
    void Idle()
    {
      idle_num++;
      pthread_cond_wait(&cond,&lock);
      idle_num--;
    }
    void Wakeup()
    {
      pthread_cond_signal(&cond);
    }
    Task PopTask()
    {
      Task t=task_queue.front();
      task_queue.pop();
      return t;
    }
    void PushTask(Task &t)
    {
      LockQueue();
      task_queue.push(t);
      UnLockQueue();
      Wakeup();
    }
    static void *ThreadRoutine(void *arg)
    {
      pthread_detach(pthread_self());
      ThreadPool *tp=(ThreadPool*) arg;
      for(;;){
        tp->LockQueue();
        if(tp->IsTaskQueueEmpty()){
          tp->Idle();
        }
        Task t=tp->PopTask();
        tp->UnLockQueue();
        std::cout<<"task is hander by:"<<pthread_self()<<std::endl;
        t.Run();
      }
    }
    ~ThreadPool()
    {
      pthread_mutex_destroy(&lock);
      pthread_cond_destroy(&cond);
    }

};
//单例模式
class singleton{
  private:
    static ThreadPool *p;
    static pthread_mutex_t lock;
   public:
      static ThreadPool *GetInsert()
      {
        if(NULL==p)
        {pthread_mutex_lock(&lock);
          if(NULL==p){
            p = new ThreadPool(5);
            p->InitThreadPool();
          }
          pthread_mutex_unlock(&lock);
        }
        return p;
      }
};
ThreadPool *singleton::p = NULL;
pthread_mutex_t singleton::lock=PTHREAD_MUTEX_INITIALIZER;

#endif


