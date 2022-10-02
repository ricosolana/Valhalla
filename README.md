# Valhalla server

This is the primary server branch with planned support for Lua modding.

TODO:
	In Valheim C#, "static" instances are instantiated via gameobjects
	which normally have a lifetime in Unity.

	I plan to streamline this usage into namespaces?

	A system which is constantly utilized for nothing but a container
	for other subsystems should not need a lifetime if its active for like all
	the time.

	tldr,
	ZNet, ZRoutedRPC, ZoneSystem, ZDOMan, ...
	should not be treated as objects but instead as namespaces
	encapsulations for functionality rather than having a lifetime