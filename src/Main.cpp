// main.cpp
#include <steam/steamnetworkingsockets.h>

int main(int argc, char **argv) {
    SteamNetworkingIdentity identity;
    
    identity.ParseString("str:peer-server");

	return 0;
}
