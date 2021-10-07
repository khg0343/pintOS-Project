CSED312 OS Lab 1
================
Final Report
----------------
컴퓨터공학과
20180085 송수민 20180373 김현지
------------------------------
# I. Implementation of Alarm Clock
## Analysis
>Alarm Clock이란 Thread가 Sleep상태일 때, 이를 깨워 Running state로 만드는 기능이다. 현재 구현 된 Alarm  Clock을 보자.

    void timer_sleep (int64_t ticks) 
    {
        int64_t start = timer_ticks ();
        ASSERT (intr_get_level () == INTR_ON);
        while (timer_elapsed (start) < ticks) 
        thread_yield ();
    }


