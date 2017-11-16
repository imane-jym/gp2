#ifndef __SINGLETON_H___
#define __SINGLETON_H___

#include "zthread/ZThread.h"
#include "zthread/FastMutex.h"

template<typename T>
class Singleton
{
	public:
		static T* getInstance ()
		{
		//	mutex_.lock();
			if (instance_ == NULL)
			{
				//First check is more effective for program. Maybe multi threads run here, So we need to check again
				ZThread::Guard<ZThread::FastMutex> guard(lock_);
				if (instance_ == NULL)
					instance_ = new T();
			}
		//	mutex_.unlock();

			return instance_;
		}

		static T& GetSystem()
		{
			return *getInstance();
		}

		virtual ~Singleton ()
		{
			instance_ = NULL;
		}
	private:
		static T* instance_;
		//static QMutex mutex_;
		static ZThread::FastMutex lock_;
	protected:
		Singleton () {;}
};
template<typename T> T* Singleton<T>::instance_ = NULL;
template<typename T> ZThread::FastMutex Singleton<T>::lock_;
//template<typename T> QMutex Singleton<T>::mutex_;

#endif /**/
