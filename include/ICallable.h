#pragma once

#include "Method.h"

template<typename T>
class ICallable {
private:
	void Register(const char* name, IMethod<T>* method);

public:

	/**
		* @brief Register a static method for remote invocation
		* @param name function name to register
		* @param method ptr to a static function
	*/
	template<class ...Args>
	auto Register(const char* name, void(*f)(T, Args...)) {
		return Register(name, new MethodImpl(f));
	}

	/**
		* @brief Register an instance method for remote invocation
		* @param name function name to register
		* @param object the object containing the member function
		* @param method ptr to a member function
	*/
	template<class C, class ...Args>
	auto Register(const char* name, C* object, void(C::* f)(T, Args...)) {
		return Register(name, new MethodImpl(object, f));
	}

	/**
		* @brief Invoke a remote function
		* @param name function name to invoke
		* @param ...types function parameters
	*/
	template <typename... Types>
	void Invoke(const char* method, Types... params) {
		if (m_socket->GetConnectivity() == Connectivity::CLOSED)
			return;

		auto pkg(PKG());
		auto stable = VUtils::GetStableHashCode(method);
		pkg->Write(stable);
#ifdef RPC_DEBUG
		pkg->Write(method);
#endif // RPC_DEBUG
		NetPackage::Serialize(pkg, std::move(params)...); // serialize
		SendPackage(pkg);
	}
};

// why is this needed?
// there should be another way that is more performant and not ugly like this
