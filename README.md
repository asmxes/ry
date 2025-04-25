# RY.HPP

Minimum dependency, header only, useful stuff I use for my projects.

## Usage

Include the header wherever you like, in your main.cpp at the end of your includes define this:

```cpp
#define RY_IMPL
#include "ry/ry.hpp"
```

otherwise implementations wont be defined.

### Logger

Export a env variable with this format:

```shell
export RY_LOG_LEVEL_log_name=1
```

or you can enable all logs like this:

```shell
export RY_LOG_LEVEL_ALL=1
```

then call the logger like this

```cpp
RY_LOG(<log_name>, <log>)
```

You can also export a env variable that defines the path to a file where the logs will be written, like this:

```shell
export RY_LOG_PATH=/Users/asmxes/Desktop/my_app.log
```

### Event bus

```cpp
    TestClass a;
    u32 number = 1339;

    RY_SUBSCRIBE("test_event", &c_func_callback)
    RY_SUBSCRIBE("test_event", &a, &TestClass::callback_example)
    RY_PUBLISH("test_event", &number)

    RY_PUBLISH_ASYNC("test_event", &number)
```

Event names will eventually fallback to a `unsigned int/u32 event_id` variable.

### Module

```cpp
class config : public RY_MODULE
{
	ry_event_contract_id config_id = {};
	nlohmann::json data = nlohmann::json();

	virtual std::string get_name();
	virtual void update();
	virtual void start();
	virtual void stop();

	void on_fetch();

public:

	static nlohmann::json get_data();
};
```

Instantiating a variable type `config` will add it to the module list, you can then call:

```cpp
    RY_MODULE_START_ALL;
    while (!can_quit)
    {
        RY_PUBLISH("fetch", nullptr);
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    RY_MODULE_STOP_ALL;
```

To start and stop all modules.

### TODO

Pass N arguments as events instead of a buffer pointer.
