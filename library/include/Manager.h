#pragma once

namespace avledet::util {

    template<typename T>
    class IManager
    {
    public:
        IManager(const IManager&) = delete;
        IManager& operator=(const IManager&) = delete;

        IManager(IManager&&) = delete;
        IManager& operator=(IManager&&) = delete;

        static T& instance() {
            static T inst;
            return inst;
        }

    protected:
        IManager() = default;
    };

}
