 

#ifndef _UDPLOG_SINGLETON_H_
#define _UDPLOG_SINGLETON_H_


#include <stdio.h>


template<typename T>
class CSingleton
{
public:
	//����ʵ��
    static T& CreateInstance()
	{
		return GetInstance();
	}
	//��ȡʵ��
	static T& GetInstance()
	{
		if (NULL == ms_pInstance)
		{
			ms_pInstance = new T();
		}

		return *ms_pInstance;
	}
	//����ʵ��
	static void DestroyInstance()
	{
		if (NULL != ms_pInstance)
		{
			delete ms_pInstance;
			ms_pInstance = NULL;
		}
	}

	//�����µ�ʵ�����ɵ�ʵ��������
	static T* SwapInstance(T* new_one)
	{
		T* old_one = ms_pInstance;
		ms_pInstance = new_one;
		return old_one;
	}

protected:
	static T	*ms_pInstance;

protected:
	CSingleton()
	{
	}
	virtual ~CSingleton()
	{
	}

private:
    // Prohibited actions
    CSingleton(const CSingleton &);
    CSingleton& operator = (const CSingleton &);

};

template<typename T>
T* CSingleton<T>::ms_pInstance = NULL;


#endif
