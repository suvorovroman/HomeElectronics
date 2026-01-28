#include <avr/sleep.h>

/** Task. */
class task
{
  public:

    task():Next(NULL){}

    virtual void operator()() = 0;

    /** Task queue. */
    class queue
    { 
      public:

        /** Create empty queue. */
        queue():Tail(NULL){}

        task* head()
        {
          return Tail;
        }

        task* tail()
        {
          return Tail->Next;
        }

        /** Put task in queue.
         *  
         *  Puts task at the queue if it is not yet in any queue and returns pointer to the task. Does nothing and return NULL
         *  if task already in a queue.
         *  
         *  \param[in]  _t  Task to put into the queue.
         *  \return Pointer to the task or NULL if task already is in queue.
         */
        task* put(task *t)
        {
          if(t->Next == NULL)
          {
            if(Tail)
            {
              t->Next = Tail->Next;
              Tail->Next = t;
            }
            else
              t->Next = t;
            Tail = t;
            return t;     // t is scheduled for execution
          }
          else
            return NULL;  // t is already in a queue and is not scheduled
        }

        /** Removes head task from queue and returns pointer to it.
         *  
         *  Does not check if queue is not empty and in the case result is unpredictable.
         *  
         *  \return Pointer to removed task;
         */
        task* get()
        {
          auto t = Tail->Next;
          if(t == Tail)
            Tail = NULL;
          else
            Tail->Next = t->Next;
          t->Next = NULL;
          return t;
        }

      private:
  
        /* 
         *  Task queue is a circular linked list.
         *              Tail  
         *                | 
         *                v
         *  T1->T2->...->Tn
         *  ^             |
         *  +-------------+
         */
         task *Tail;
      
    };

    /** Task scheduler. */
    class scheduler
    {
    public:

	scheduler():ISRFlag(false){}

        task* put(task* t)
        {
          return Queue.put(t);
        }

        task* putISR(task* t)
        {
          t = ISRQueue.put(t);
          if(t)
            ISRFlag = true;
          return t;
        }

      /** \brief Executes next task from queue.
	  Takes and executes a task from ISR queue or main queue. If both queues are empty and _sleep parameter is true
	  puts device into sleep mode specified by parameter _mode.

	  \param[in]	_sleep	Put device into sleeping mode if ISR and main queues are empty.
	  \param[in]	_mode	Sleeping mode.
       */
      void execute(bool _sleep = false, uint8_t _mode = SLEEP_MODE_IDLE)
      {
	if(ISRFlag)
	  {
	    (*ISRQueue.tail())();
	    noInterrupts();
	    ISRQueue.get();
	    if(ISRQueue.head() == NULL)
	      ISRFlag = false;
	    interrupts();
	  }
	else
	  if(Queue.head())
	    (*Queue.get())();
	  else
	    if(_sleep)
	      {
		noInterrupts();
		if(ISRFlag)
		  /* ISRQueue contains a task to process. Continue processing. */
		  interrupts();
		else
		  {
		    /* ISRQueue really empty. Go idle. */
		    set_sleep_mode(_mode);
		    sleep_enable();
		    interrupts();
		    sleep_cpu();
		    sleep_disable();
		  }
	      }
      }

    private:
	queue Queue;
        queue ISRQueue;

    public:
        byte  ISRFlag;
        
    };

  private:

    task *Next;
  
};
