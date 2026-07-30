#pragma once

namespace avledet::util {

    template<typename T>
    class IManager
    {
    private:
        static inline T inst;

    public:
        IManager(const IManager&) = delete;
        IManager& operator=(const IManager&) = delete;

        IManager(IManager&&) = delete;
        IManager& operator=(IManager&&) = delete;

        static T& instance() {
            return inst;
        }

    protected:
        IManager() = default;
    };

}
