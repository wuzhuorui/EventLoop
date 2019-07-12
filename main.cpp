#include<thread>
#include<mutex>
#include<iostream>
#include<vector>
#include<map>
#include<memory> 
#include <condition_variable>

typedef std::function<void(void)> Functor;

class TaskCenter
{
	public:
		static TaskCenter& Instance()
		{
			static TaskCenter m_instance;
			return m_instance;	
		}

		void PushTask(int id,Functor f){
			std::lock_guard<std::mutex>(center[id].first.first);
			center[id].second.push_back(f);
			center[id].first.second.notify_all();
		}

		void Pop(int id)
		{
			std::unique_lock<std::mutex> lk(center[id].first.first);
			if (center[id].second.empty()){
				center[id].first.second.wait(lk,[&](){
						return !center[id].second.empty();
						});
			}
			for (auto f : center[id].second)
			{
				f();
			}
			center[id].second.clear();
			lk.unlock();
		}

	private:
		std::map<int,std::pair<std::pair<std::mutex,std::condition_variable>,std::vector<Functor>>> center;
		TaskCenter(){};
		TaskCenter(const TaskCenter&);
		TaskCenter& operator=(const TaskCenter&);
};

template <typename EventDerived>
class EventBase{
	public:
	static void Send();
	static void Regrister(int ,Functor);
	static std::map<int,Functor> map_functor;
	static std::mutex m;
};

template <typename EventDerived> 
std::map<int,Functor> EventBase<EventDerived>::map_functor;

template <typename EventDerived> 
std::mutex EventBase<EventDerived>::m;

template <typename EventDerived>
void EventBase<EventDerived>::Send()
{
	std::lock_guard<std::mutex> lk(m);
	for (auto& elem : map_functor)
	{
		std::cout << "Send Event";
		TaskCenter::Instance().PushTask(elem.first,elem.second);
	}
}

template <typename EventDerived>
void EventBase<EventDerived>::Regrister(int id,Functor f)
{
	std::lock_guard<std::mutex> lk(m);
	std::cout << "Regrister : " << id << std::endl;
	map_functor[id] = f;
}


class Event1 : public EventBase<Event1>{
};

class Event2 : public EventBase<Event2>{
};

void functor1(void)
{
	std::cout << "do functor1" << std::endl;
}

void functor2(void)
{
	std::cout << "do functor2" << std::endl;
}


void fun1()
{
	Functor f = functor1;
	Functor f2 = functor2;
	Event1::Regrister(1, f	);
	Event2::Regrister(1,f2);

	while(true)
	{
		TaskCenter::Instance().Pop(1);
	}

}
void fun2()
{
	int i = 0;
	while(std::cin >> i)
	{
		if (i == 0)
			Event1::Send();
		else if (i==2)
			Event2::Send();
	}

}

int main()
{
	std::thread t1(fun1);
	std::thread t2(fun2);
	
	t1.join();
	t2.join();
	return 0;
}
