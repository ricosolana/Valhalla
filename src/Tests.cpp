#include "Tests.h"
#include "ZDOManager.h"

class TestSocket : public ISocket {
public:
    TestSocket() {}
    void Close(bool) {}
    void Update() {}
    void Send(BYTES_t) {}
    std::optional<BYTES_t> Recv() { return std::nullopt; }
    std::string GetHostName() const { return "crzi"; }
    std::string GetAddress() const { return "127.0.0.1"; }
    bool Connected() const { return true; }
    unsigned int GetSendQueueSize() const { return 5000; }
    unsigned int GetPing() const { return 15; }
};

void VHTest::Test_ZDOConnectors() {

    PrefabManager()->Init();
    
    WorldManager()->RetrieveWorld("betatest0p216p5", "fail")->LoadFileDB();
    
    //VH_SETTINGS.worldName = "betatest0p216p5";
    //VH_SETTINGS.worldSeed = "fail";
    //WorldManager()->

    Peer peer(std::make_shared<TestSocket>());

    ZDOManager()->SendAllZDOs(peer);
}
