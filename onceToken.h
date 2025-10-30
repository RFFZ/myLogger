#include <functional>
#include <type_traits>

class onceToken
{
public:
    using task = std::function<void(void)>;

    template <typename FUNC>
    onceToken(const FUNC &onConstructed, task onDestracted = nullptr)
    {
        onConstructed();
        _onDestracted = onDestracted;
    }

    onceToken(std::nullptr_t, task onDestracted = nullptr)
    {
        _onDestracted = onDestracted;
    }

    ~onceToken()
    {
        if (_onDestracted)
            _onDestracted();
    }

private:
    onceToken() = delete;
    onceToken(const onceToken &) = delete;
    onceToken(onceToken &&) = delete;
    onceToken &operator=(const onceToken &) = delete;
    onceToken &operator=(onceToken &&) = delete;

private:
    task _onDestracted;
};