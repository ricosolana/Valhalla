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

void VHTest::Test_Quaternion() {
    assert(Quaternion::IDENTITY.EulerAngles() == Vector3f::Zero());
}

void VHTest::ZDO_Sets(ZDO& zdo) {
    zdo.Set("my key", std::string("my value"));
    zdo.Set("my int", 10);
    zdo.Set("large prime", 2147483647);
    zdo.Set("pi", 3.141592654f);
    zdo.Set("my vec", Vector3f(0, 1, 0));
    zdo.Set("my quat", Quaternion(0, 0, 0, 1));

    zdo.SetPosition(Vector3f(0, 5, 100));
    zdo.SetRotation(Quaternion(1, 0, 0, 0));
}

void VHTest::Test_ZDO_Gets(ZDO& zdo) {    
    assert(zdo.GetString("my key") == "my value");
    assert(*zdo.Get<int32_t>("my int") == 10);
    assert(*zdo.Get<int32_t>("large prime") == 2147483647);
    assert(*zdo.Get<float>("pi") == 3.141592654f);
    assert(*zdo.Get<Vector3f>("my vec") == Vector3f(0, 1, 0));
    assert(*zdo.Get<Quaternion>("my quat") == Quaternion(0, 0, 0, 1));

    assert(zdo.Position() == Vector3f(0, 5, 100));
    //assert(zdo.Rotation().LengthSquared() == Quaternion(1, 0, 0, 0).LengthSquared());

    // TODO Quaternions are screwed up still, luckily everything else now works
    //  The test was wrong, (and thus the usage, for some reason char* was being interpreted as a int?)
}

void VHTest::Test_ZDO_SetsGets() {
    ZDO zdo;
    ZDO_Sets(zdo);
    Test_ZDO_Gets(zdo);
}

void VHTest::Test_ZDO_LoadSave() {
    PrefabManager()->Init();

    BYTES_t bytes;

    // ZDO's now require correct pooling
    //  they cannot be tested independent of ZDOManager
    //  (due to member optimizations)
    // This is a somewhat sensible tradeoff
    //  because of the ZDO's being directly tied with ZDOManager

    //{
        auto&& zdo = ZDOManager()->Instantiate(Hashes::Object::Abomination, Vector3f::Zero()).get();

        ZDO_Sets(zdo);
        DataWriter writer(bytes);

        zdo.Pack(writer, false);
    //}

    auto&& zdo2 = ZDOManager()->Instantiate(Hashes::Object::Abomination, Vector3f::Zero()).get();

    DataReader reader(bytes);
    zdo2.Unpack(reader, VConstants::WORLD);

    assert(ZDO::ZDO_MEMBERS.size() == 2);

    int i = 0;
    for (auto&& e : ZDO::ZDO_MEMBERS) {
        for (auto&& z : e.second) {
            i++;
        }
    }

    Test_ZDO_Gets(zdo2);
}
