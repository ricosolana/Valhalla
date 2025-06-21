#include "ModManager.h"
#include "Quaternion.h"
#include <sol/base_traits.hpp>
#include <sol/forward.hpp>
#include <sol/property.hpp>

#if VH_IS_ON(VH_USE_MODS)

//was readonly
//template <typename T>
//struct constant_wrapper : sol::detail::ebco<T> {
//private:
//    using base_t = sol::detail::ebco<T>;
//
//public:
//    using base_t::base_t;
//
//    operator T&() {
//        return base_t::value();
//    }
//    operator const T&() const {
//        return base_t::value();
//    }
//};

namespace sol {
    // TODO test this
    //  Usage is intended for readonly / immutable static constants
    template<typename V>
    inline auto constant(V &&v)
    {
        return readonly_wrapper<meta::unqualified_t<decltype(v)>>(v);
    }
}// namespace sol

void IModManager::load_userdata_quaternion()
{
    LOG_DEBUG(VH_LOGGER, "Initializing API types - quaternion");

    this->new_usertype<Quaternion>(
            "Quaternion", sol::constructors<Quaternion(), Quaternion(float, float, float, float)>(),

            // TODO test
            //"IDENTITY", sol::constant(Quaternion::IDENTITY),

            "IDENTITY", sol::var(Quaternion::IDENTITY),//sol::property([]() {Quaternion::IDENTITY}),
            "x", sol::readonly(&Quaternion::x), "y", sol::readonly(&Quaternion::y), "z",
            sol::readonly(&Quaternion::z), "w", sol::readonly(&Quaternion::w), sol::meta_function::index,
            [](Quaternion &self, std::size_t index, sol::state_view view) {
                return index == 1   ? sol::make_object(view, self.x)
                       : index == 2 ? sol::make_object(view, self.y)
                       : index == 3 ? sol::make_object(view, self.z)
                       : index == 4 ? sol::make_object(view, self.w)
                                    : sol::lua_nil;
            },
            "length_squared", sol::property(&Quaternion::length_squared), "xyz",
            sol::property(&Quaternion::xyz), "euler_angles", sol::property(&Quaternion::euler_angles), "dot",
            sol::property(&Quaternion::dot),
            //statics
            "euler",
            sol::overload(sol::resolve<Quaternion(float, float, float)>(&Quaternion::euler),
                          sol::resolve<Quaternion(Vector3f)>(&Quaternion::euler)),
            //"look_rotation"...
            sol::meta_function::multiplication,
            sol::resolve<Quaternion(Quaternion) const>(&Quaternion::operator*),
            //"multiply", sol::resolve(Q
            sol::meta_function::equal_to, &Quaternion::operator==);

}// namespace avledet::api

#endif
