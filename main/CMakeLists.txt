# c++23 needed for std::to_underlying...
if (ESP_PLATFORM)
	set(CMAKE_CXX_STANDARD 23)

	idf_component_register(SRCS "NetSocket.cpp" "NetAcceptor.cpp" "WorldManager.cpp" "ZoneManager.cpp" "Peer.cpp" "NetManager.cpp" "Main.cpp"
						"Quaternion.cpp" "RouteManager.cpp" "ValhallaServer.cpp" "Vector.cpp" "VUtils.cpp" 
						"VUtilsMath.cpp" "VUtilsMathf.cpp" "VUtilsRandom.cpp" "VUtilsResource.cpp" "VUtilsString.cpp"
						"ZDO.cpp" "ZDOID.cpp" "ZDOManager.cpp" 
						INCLUDE_DIRS "./include"
						#PRIV_REQUIRES unordered_dense
	)
else()
	find_package(asio CONFIG REQUIRED)

	add_executable (MyValhalla 
		"NetSocket.cpp" "NetAcceptor.cpp" "WorldManager.cpp" "ZoneManager.cpp" "Peer.cpp" "NetManager.cpp" "Main.cpp"
		"Quaternion.cpp" "RouteManager.cpp" "ValhallaServer.cpp" "Vector.cpp" "VUtils.cpp" 
		"VUtilsMath.cpp" "VUtilsMathf.cpp" "VUtilsRandom.cpp" "VUtilsResource.cpp" "VUtilsString.cpp"
		"ZDO.cpp" "ZDOID.cpp" "ZDOManager.cpp" 
	)
	
	set(ANKERL_UNORDERED_DENSE_ROOT_DIR "C:/Users/rico/Documents/Visual Studio 2022/Libraries/unordered_dense")

	target_include_directories(MyValhalla
		PUBLIC ../include
		#PUBLIC ankerl::unordered_dense
		${ANKERL_UNORDERED_DENSE_ROOT_DIR}/include
	)

	target_link_libraries(MyValhalla
		PRIVATE asio::asio
	)
	
	set_property(TARGET MyValhalla PROPERTY CXX_STANDARD 23)

	add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
	
	file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../data
		DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

	#target_compile_features(MyValhalla PRIVATE cxx_std_23)
endif()