/*
 * �����嵥���¼�����
 *
 * �������ᴴ��3����̬�̼߳���ʼ��һ����̬�¼�����
 * һ���̵߳����¼��������Խ����¼���
 * һ���̶߳�ʱ�����¼� (�¼�3)
 * һ���̶߳�ʱ�����¼� (�¼�5)
 */
#include <rtthread.h>
#include <time.h>
#include "tc_comm.h"

/* ָ���߳̿��ƿ��ָ�� */
static rt_thread_t tid1 = RT_NULL;
static rt_thread_t tid2 = RT_NULL;
static rt_thread_t tid3 = RT_NULL;

/* �¼����ƿ� */
static struct rt_event event;

/* �߳�1��ں��� */
static void thread1_entry(void *param)
{
    rt_uint32_t e;

    while (1)
    {
        /* receive first event */
        if (rt_event_recv(&event, ((1 << 3) | (1 << 5)),
                          RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR,
                          RT_WAITING_FOREVER, &e) == RT_EOK)
        {
            rt_kprintf("thread1: AND recv event 0x%x\n", e);
        }

        rt_kprintf("thread1: delay 1s to prepare second event\n");
        rt_thread_delay(10);

        /* receive second event */
        if (rt_event_recv(&event, ((1 << 3) | (1 << 5)),
                          RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                          RT_WAITING_FOREVER, &e) == RT_EOK)
        {
            rt_kprintf("thread1: OR recv event 0x%x\n", e);
        }

        rt_thread_delay(5);
    }
}

/* �߳�2��ں��� */
static void thread2_entry(void *param)
{
    while (1)
    {
        rt_kprintf("thread2: send event1\n");
        rt_event_send(&event, (1 << 3));

        rt_thread_delay(10);
    }
}

/* �߳�3��ں��� */
static void thread3_entry(void *param)
{
    while (1)
    {
        rt_kprintf("thread3: send event2\n");
        rt_event_send(&event, (1 << 5));

        rt_thread_delay(20);
    }
}

int event_simple_init()
{
    /* ��ʼ���¼����� */
    rt_event_init(&event, "event", RT_IPC_FLAG_FIFO);

    /* �����߳�1 */
    tid1 = rt_thread_create("t1",
                            thread1_entry, RT_NULL, /* �߳������thread1_entry, ��ڲ�����RT_NULL */
                            THREAD_STACK_SIZE, THREAD_PRIORITY, THREAD_TIMESLICE);
    if (tid1 != RT_NULL)
        rt_thread_startup(tid1);
    else
        tc_stat(TC_STAT_END | TC_STAT_FAILED);

    /* �����߳�2 */
    tid2 = rt_thread_create("t2",
                            thread2_entry, RT_NULL, /* �߳������thread2_entry, ��ڲ�����RT_NULL */
                            THREAD_STACK_SIZE, THREAD_PRIORITY, THREAD_TIMESLICE);
    if (tid2 != RT_NULL)
        rt_thread_startup(tid2);
    else
        tc_stat(TC_STAT_END | TC_STAT_FAILED);

    /* �����߳�3 */
    tid3 = rt_thread_create("t3",
                            thread3_entry, RT_NULL, /* �߳������thread3_entry, ��ڲ�����RT_NULL */
                            THREAD_STACK_SIZE, THREAD_PRIORITY, THREAD_TIMESLICE);
    if (tid3 != RT_NULL)
        rt_thread_startup(tid3);
    else
        tc_stat(TC_STAT_END | TC_STAT_FAILED);

    return 0;
}

#ifdef RT_USING_TC
static void _tc_cleanup()
{
    /* �����������������󣬽������л��������̣߳�����Ӧ�ж� */
    rt_enter_critical();

    /* ɾ���߳� */
    if (tid1 != RT_NULL && tid1->stat != RT_THREAD_CLOSE)
        rt_thread_delete(tid1);
    if (tid2 != RT_NULL && tid2->stat != RT_THREAD_CLOSE)
        rt_thread_delete(tid2);
    if (tid3 != RT_NULL && tid3->stat != RT_THREAD_CLOSE)
        rt_thread_delete(tid3);

    /* ִ���¼��������� */
    rt_event_detach(&event);

    /* ���������� */
    rt_exit_critical();

    /* ����TestCase״̬ */
    tc_done(TC_STAT_PASSED);
}

int _tc_event_simple()
{
    /* ����TestCase����ص����� */
    tc_cleanup(_tc_cleanup);
    event_simple_init();

    /* ����TestCase���е��ʱ�� */
    return 100;
}
/* ����������finsh shell�� */
FINSH_FUNCTION_EXPORT(_tc_event_simple, a simple event example);
#else
/* �û�Ӧ����� */
int rt_application_init()
{
    event_simple_init();

    return 0;
}
#endif
