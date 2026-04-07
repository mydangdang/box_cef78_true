#pragma once
#include "queue"
#include "windows.h"

class CLightLock
{
public:
	CLightLock()
	{
		InitializeCriticalSectionAndSpinCount(&cs_, 5000);
	}
	~CLightLock()
	{
		DeleteCriticalSection(&cs_);
	}
public:
	void Lock()
	{
		EnterCriticalSection(&cs_);
	}
	void UnLock()
	{
		LeaveCriticalSection(&cs_);
	}
private:
	CRITICAL_SECTION cs_;
};

enum BufferReadRES
{
	BUFFER_EMPTY = 0,
	BUFFER_SWAP = 1,
	BUFFER_POPOK = 2
};

template<typename T>
struct SyncQueue
{
	SyncQueue()
	{
	}
	~SyncQueue()
	{
		buffer.empty();
	}
	void push(T buf)
	{
		buffer.push(buf);
	}
	T pop()
	{
		T buf = buffer.front();
		buffer.pop();
		return buf;
	}
	int size()
	{
		return buffer.size();
	}
	void clear()
	{
		std::queue<T> empty;
		std::swap( buffer, empty );
	}
	std::queue<T> buffer;
};

//多个生产者 必须在push时加锁
/*针对多个消费者:
可以在取的时候加锁  这样每个消费者 在访问的时候都要等待锁同步
也可以增加一个调度线程,负责将push来的数据 调度到各个独自的线程队列
这样只有一个线程调度push的任务, 而真实的消费者在收到调度的任务后 也不需要加锁
*/

/*
针对锁的使用,为了提高性能 减少模式切换与等待     减少锁或使用的粒度 注意减少切换 使用同类原子锁等 可以提高性能;
*/

/*
IOCP考虑:

I/O时耗过长 采用的几种方案对比到一般逻辑处理

1:设备对象通知
  为每个业务请求创建一个线程,每个线程等待请求的设备对象 返回完成通知 业务继续往下走
  只能称为多线程 并非异步 逻辑代码依旧会停止执行等待IO或者说消费者执行完毕

2:事件内核通知
  同1  但是业务在请求设备对象或者说消费者后 继续执行 只在必要数据那设置 事件 等待设备或消费者完成后激活事件
  缺点:简单异步,但是一个线程最多 WaitForMultipleObjects 的事件数目有限  64为几线 ,如果并发量过大,则需要创建更多线程

3:APC回调通知
  同2 不过不是等待激活事件 在逻辑上等待 设置给 设备或者消费者的回调函数  解决了2中的 线程过多问题(设置回调后基本请求线程就结束了等待回调)
  缺点:回调时线程上下文为切换 需要考虑同步,而且针对单个线程的回调 会导致线程繁忙,负载不均衡

4:IOCP异步
  创建好CPU支持的线程数  请求全部发送到公共队列中 ,消费者拿到后也统一回馈到一个公共队列 告诉请求者 OK反馈数据已存在 让请求者自己去取

  

*/
template<typename T>
class CDoubleBufferQueue
{
public:
	CDoubleBufferQueue()
	{
		m_bMultiProduer = true;
		m_pCurPushBuffer = &m_bufferPush;
		m_pCurPopBuffer = &m_bufferPop;
	}
	~CDoubleBufferQueue()
	{

	}
public:
	void SetSingleProducer(bool bSingle = true)
	{
		m_bMultiProduer = false;
	}
	BufferReadRES ExchangeQueue()
	{
		if(m_bMultiProduer)
			bufferLock.Lock();
		SyncQueue<T>* tmp = m_pCurPopBuffer;
		m_pCurPopBuffer = m_pCurPushBuffer;
		m_pCurPushBuffer = tmp;

		if(!m_pCurPopBuffer->size()&&!m_pCurPushBuffer->size())
		{
			if(m_bMultiProduer)
				bufferLock.UnLock();
			return BUFFER_EMPTY;
		}
		if(m_bMultiProduer)
			bufferLock.UnLock();
		return BUFFER_SWAP;
	}
	void PushBuffer(T buf)
	{
		if(m_bMultiProduer)
			bufferLock.Lock();
		m_pCurPushBuffer->push(buf);
		if(m_bMultiProduer)
			bufferLock.UnLock();
	}
	BufferReadRES PopBuffer(T & buf)
	{
		int a = 0;
		if(m_pCurPopBuffer->size())
		{
			buf = m_pCurPopBuffer->pop();
			return BUFFER_POPOK;
		}
		else
			return ExchangeQueue();
		return BUFFER_EMPTY;
	}
	void ClearBuffer()
	{
		m_bufferPush.clear();
		m_bufferPop.clear();
	}
protected:
	bool m_bMultiProduer;
	CLightLock bufferLock;

	SyncQueue<T> m_bufferPush;
	SyncQueue<T> m_bufferPop;

	SyncQueue<T>* m_pCurPushBuffer;
	SyncQueue<T>* m_pCurPopBuffer;
};