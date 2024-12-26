#ifndef RY_HPP
#define RY_HPP

// TYPES ----------

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long s64;
typedef unsigned long u64;
typedef float f32;
typedef double f64;
typedef void* ptr;

// LOG ----------

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>

#define RY_LOG_COLOR_RESET   "\033[0m"
#define RY_LOG_COLOR_RED     "\033[31m"
#define RY_LOG_COLOR_GREEN   "\033[32m"
#define RY_LOG_COLOR_YELLOW  "\033[33m"
#define RY_LOG_COLOR_BLUE    "\033[34m"
#define RY_LOG_COLOR_MAGENTA "\033[35m"
#define RY_LOG_COLOR_CYAN    "\033[36m"

inline std::ostream& _RY_GET_LOG_OUTPUT(bool is_error = false)
{
    static std::ofstream file_stream;
    const char* log_path = std::getenv("RY_LOG_PATH");

    if(file_stream.is_open())
    {
        return file_stream;
    }

    if(!log_path)
    {
        return is_error ? std::cerr : std::cout;
    }
    else
    {
        file_stream.open(log_path, std::ios::app);
        if(!file_stream.is_open())
        {
            return is_error ? std::cerr : std::cout;
        }
        else
        {
            return file_stream;
        }
    }
}

#define RY_LOG(level, message) {if (std::getenv(std::string(std::string("RY_LOG_LEVEL_") + std::string(level)).c_str()) || std::getenv("RY_LOG_LEVEL_ALL")){_RY_GET_LOG_OUTPUT() << RY_LOG_COLOR_CYAN << "● [ " << level << " ] » " << RY_LOG_COLOR_RESET << message << std::endl;}}
#define RY_ERR(message) {_RY_GET_LOG_OUTPUT(true) << RY_LOG_COLOR_RED << "✘ [ " << __FUNCTION__ << " ] » " << RY_LOG_COLOR_RESET << message << std::endl;}

// HASH ----------

inline u8 _RY_CHAR_TO_LOWER(u8 input)
{
	if (static_cast<u8>(input - 0x41) > 0x19u) return input;
	else return input + 0x20;
}

inline u32 RY_HASH_STRING(std::string s)
{
	u32 hash = 0x811c9dc5;
	const char* str = s.c_str();

	for (u32 i = 0u; str[i]; i++)
	{
		hash = 16777619 * (hash ^ _RY_CHAR_TO_LOWER(str[i]));
	}

	return hash;
}

// EVENT ----------

#include <unordered_map>
#include <queue>

typedef u64 ry_event_contract_id;
typedef u32 ry_event_id;

class _ry_event_manager
{
    struct _callback_enity
    {
        ptr obj;
        ptr func;
    };

    template <typename O, typename F>
    struct _typed_proxy
    {
        O _o;
        F _f;

        _typed_proxy(O _o_in, F _f_in) : _o(_o_in), _f(_f_in){}
    };

    std::unordered_map<ry_event_id, std::vector<_callback_enity*> > _subscribers;

    std::queue<std::pair<ry_event_id, ptr> > _async_events;

    _ry_event_manager();

    void _async_publisher();

    template <typename O, typename F>
    ptr _create_proxy(O object, F fun);

    public:

    template <typename O, typename F>
    ry_event_contract_id subscribe(ry_event_id event_id, O object, F func);

    template <typename F>
    ry_event_contract_id subscribe(ry_event_id event_id, F func);

    void unsubscribe(ry_event_id event_id, ry_event_contract_id contract_id);

    void publish(ry_event_id event_id, ptr data);

    void publish_async(ry_event_id event_id, ptr data);

    static _ry_event_manager* get_singleton();
};

#define _RY_EVENT_MANAGER _ry_event_manager::get_singleton
#define RY_SUBSCRIBE(event_name, ...)  {_RY_EVENT_MANAGER()->subscribe(RY_HASH_STRING(event_name), __VA_ARGS__);}
#define RY_PUBLISH(event_name, data) {_RY_EVENT_MANAGER()->publish(RY_HASH_STRING(event_name), data);}
#define RY_PUBLISH_ASYNC(event_name, data) {_RY_EVENT_MANAGER()->publish_async(RY_HASH_STRING(event_name), data);}

template <typename O, typename F>
ptr _ry_event_manager::_create_proxy(O object, F fun)
{
    using proxy = _typed_proxy<O, F>;

    auto* pack = new proxy(object, fun);
    return static_cast<ptr>(pack);
}

template <typename O, typename F>
ry_event_contract_id _ry_event_manager::subscribe(ry_event_id event_id, O object, F func)
{
    _callback_enity* proxy = reinterpret_cast<_callback_enity*>(this->_create_proxy(object, func));
    this->_subscribers[event_id].emplace_back(proxy);
    return reinterpret_cast<ry_event_contract_id>(proxy);
}

template <typename F>
ry_event_contract_id _ry_event_manager::subscribe(ry_event_id event_id, F func)
{
    _callback_enity* proxy = new _callback_enity();
    proxy->obj = nullptr;
    proxy->func = reinterpret_cast<ptr>(func);

    this->_subscribers[event_id].emplace_back(proxy);
    return reinterpret_cast<ry_event_contract_id>(proxy);
}

#endif

#ifdef RY_IMPL

#include <thread>
#include <chrono>

_ry_event_manager::_ry_event_manager()
{
    std::thread(&_ry_event_manager::_async_publisher, this).detach();
}

void _ry_event_manager::_async_publisher()
{
    RY_LOG("_ry_event_manager", "Started _async_publisher thread")

    while(true)
    {
        if(!_async_events.empty())
        {
            std::pair<ry_event_id, ptr> to_dispatch = this->_async_events.front();
            this->_async_events.pop();

            this->publish(to_dispatch.first, to_dispatch.second);
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
}

void _ry_event_manager::publish_async(ry_event_id event_id, ptr data)
{
    this->_async_events.push(std::make_pair(event_id, data));
}

void _ry_event_manager::unsubscribe(ry_event_id event_id, ry_event_contract_id contract_id)
{
    auto &subscribed_entities = this->_subscribers[event_id];
    for(auto it = subscribed_entities.begin();
        it != subscribed_entities.end();
        it++)
    {
        if(reinterpret_cast<ry_event_contract_id>(*it) == contract_id)
        {
            subscribed_entities.erase(it);
            delete (*it);
            break;
        }
    }
}

void _ry_event_manager::publish(ry_event_id event_id, ptr data)
{
    if(!data)
    {
        return;
    }

    auto found = this->_subscribers.find(event_id);

    if(found == this->_subscribers.end())
    {
        return;
    }
    else
    {
        for(auto& sub : found->second)
        {
            if(!sub->func)
            {
                continue;
            }

            if(sub->obj)
            {
                reinterpret_cast<void(*)(ptr,ptr)>(sub->func)(sub->obj, data);
            }
            else
            {
                reinterpret_cast<void(*)(ptr)>(sub->func)(data);
            }
        }
    }
}

_ry_event_manager* _ry_event_manager::get_singleton()
{
    static _ry_event_manager object;
    return &object;
}

#endif
