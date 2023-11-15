#ifndef SINGLETON_H
#define SINGLETON_H


 
template <class T>
class Singleton
{
public:
	static T* instance()
	{
		if(!_instance)
			_instance = new T();
		return _instance;
	}
	void close()
	{
		if (_instance)
		{
			delete _instance;
			_instance = 0;
		}
	}
protected:
	Singleton(){}
	~Singleton(){}
	static T* _instance;
};


template <class T>
T* Singleton<T>::_instance = nullptr;


#endif