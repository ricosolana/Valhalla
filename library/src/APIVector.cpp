#include "ModManager.h"

#if VH_IS_ON(VH_USE_MODS)

void avledet::api::init_vector(sol::table api_table) {
    LOG_INFO(VH_LOGGER, "Initializing API types - CSU::VectorX");

    api_table.new_usertype<Vector3f>("Vector3f",
        sol::constructors<Vector3f(), Vector3f(float, float, float)>(),
        "ZERO", sol::property(&Vector3f::zero),
        "x", &Vector3f::x,
        "y", &Vector3f::y,
        "z", &Vector3f::z,
        sol::meta_function::index, [](Vector3f& self, std::size_t index, sol::state_view view) { return index == 1 ? sol::make_object(view, self.x) : index == 2 ? sol::make_object(view, self.y) : index == 3 ? sol::make_object(view, self.z) : sol::lua_nil; },
        "magnitude", sol::property(&Vector3f::magnitude),
        "sq_magnitude", sol::property(&Vector3f::sq_magnitude),
        "normal", sol::property(&Vector3f::normal),
        "distance_to", &Vector3f::distance_to,
        "sq_distance_to", &Vector3f::sq_distance_to,
        "dot", &Vector3f::dot,
        "cross", &Vector3f::cross,
        sol::meta_function::addition, &Vector3f::operator+,
        sol::meta_function::subtraction, sol::resolve<Vector3f(Vector3f const&) const>(&Vector3f::operator-),
        sol::meta_function::unary_minus, sol::resolve<Vector3f() const>(&Vector3f::operator-),
        sol::meta_function::multiplication, sol::resolve<Vector3f(Vector3f const&) const>(&Vector3f::operator*),
        sol::meta_function::division, sol::resolve<Vector3f(Vector3f const&) const>(&Vector3f::operator/),
        sol::meta_function::equal_to, &Vector3f::operator==
    );

    api_table.new_usertype<Vector2f>("Vector2f",
        sol::constructors<Vector2f(), Vector2f(float, float)>(),
        "ZERO", sol::property(&Vector2f::zero),
        "x", &Vector2f::x,
        "y", &Vector2f::y,
        sol::meta_function::index, [](Vector2f& self, std::size_t index, sol::state_view view) { return index == 1 ? sol::make_object(view, self.x) : index == 2 ? sol::make_object(view, self.y) : sol::lua_nil; },
        "magnitude", sol::property(&Vector2f::magnitude),
        "sq_magnitude", sol::property(&Vector2f::sq_magnitude),
        "normal", sol::property(&Vector2f::normal),
        "distance_to", &Vector2f::distance_to,
        "sq_distance_to", &Vector2f::sq_distance_to,
        "dot", &Vector2f::dot,
        sol::meta_function::addition, &Vector2f::operator+,
        sol::meta_function::subtraction, sol::resolve<Vector2f(Vector2f const&) const>(&Vector2f::operator-),
        sol::meta_function::unary_minus, sol::resolve<Vector2f() const>(&Vector2f::operator-),
        sol::meta_function::multiplication, sol::resolve<Vector2f(Vector2f const&) const>(&Vector2f::operator*),
        sol::meta_function::division, sol::resolve<Vector2f(Vector2f const&) const>(&Vector2f::operator/),
        sol::meta_function::equal_to, &Vector2f::operator==
    );

    api_table.new_usertype<Vector2i>("Vector2i",
        sol::constructors<Vector2i(), Vector2i(std::int32_t, std::int32_t)>(),
        "ZERO", sol::property(&Vector2i::zero),
        "x", &Vector2i::x,
        "y", &Vector2i::y,
        sol::meta_function::index, [](Vector2i& self, std::size_t index, sol::state_view view) { return index == 1 ? sol::make_object(view, self.x) : index == 2 ? sol::make_object(view, self.y) : sol::lua_nil; },
        "magnitude", sol::property(&Vector2i::magnitude),
        "sq_magnitude", sol::property(&Vector2i::sq_magnitude),
        "normal", sol::property(&Vector2i::normal),
        "distance_to", &Vector2i::distance_to,
        "sq_distance_to", &Vector2i::sq_distance_to,
        "dot", &Vector2i::dot,
        sol::meta_function::addition, &Vector2i::operator+,
        sol::meta_function::subtraction, sol::resolve<Vector2i(Vector2i const&) const>(&Vector2i::operator-),
        sol::meta_function::unary_minus, sol::resolve<Vector2i() const>(&Vector2i::operator-),
        sol::meta_function::multiplication, sol::resolve<Vector2i(Vector2i const&) const>(&Vector2i::operator*),
        sol::meta_function::division, sol::resolve<Vector2i(Vector2i const&) const>(&Vector2i::operator/),
        sol::meta_function::equal_to, &Vector2i::operator==
    );

    api_table.new_usertype<Vector2s>("Vector2s",
        sol::constructors<Vector2s(), Vector2s(std::int16_t, std::int16_t)>(),
        "ZERO", sol::property(&Vector2s::zero),
        "x", &Vector2s::x,
        "y", &Vector2s::y,
        sol::meta_function::index, [](Vector2s& self, std::size_t index, sol::state_view view) { return index == 1 ? sol::make_object(view, self.x) : index == 2 ? sol::make_object(view, self.y) : sol::lua_nil; },
        "magnitude", sol::property(&Vector2s::magnitude),
        "sq_magnitude", sol::property(&Vector2s::sq_magnitude),
        "normal", sol::property(&Vector2s::normal),
        "distance_to", &Vector2s::distance_to,
        "sq_distance_to", &Vector2s::sq_distance_to,
        "dot", &Vector2s::dot,
        sol::meta_function::addition, &Vector2s::operator+,
        sol::meta_function::subtraction, sol::resolve<Vector2s(Vector2s const&) const>(&Vector2s::operator-),
        sol::meta_function::unary_minus, sol::resolve<Vector2s() const>(&Vector2s::operator-),
        sol::meta_function::multiplication, sol::resolve<Vector2s(Vector2s const&) const>(&Vector2s::operator*),
        sol::meta_function::division, sol::resolve<Vector2s(Vector2s const&) const>(&Vector2s::operator/),
        sol::meta_function::equal_to, &Vector2s::operator==
    );
    
}

#endif
