#include <filesystem>
#include <string>
#include <fstream>
#include <cassert>

namespace fs = std::filesystem;
using BYTE_t = uint8_t;



// Ease-in-out function
// t: [0, 1]
// https://www.desmos.com/calculator/tgpfii21pt
static double myfade(float t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }
static double mylerp(float t, float a, float b) { return a + t * (b - a); }
static double mygrad(int hash, float x, float y) {
    int h = hash & 15;                            // CONVERT LO 4 BITS OF HASH CODE
    float u = h < 8 ? x : y,                    // INTO 12 GRADIENT DIRECTIONS.
        v = h < 4 ? y : h == 12 || h == 14 ? x : 0;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

static BYTE_t p[] = { 151,160,137,91,90,15,
    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
    77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
    135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
    5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
    223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
    129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
    251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
    // 2nd copy
    151,160,137,91,90,15,
    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
    77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
    135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
    5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
    223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
    129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
    251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

float PerlinNoise(float x, float y) {
    int X = (int)floorf(x) & 0xFF;
    int Y = (int)floorf(y) & 0xFF;

    x -= floorf(x);
    y -= floorf(y);

    int A = p[X] + Y;
    int B = p[X + 1] + Y;

    int BB = p[p[B + 1]];
    int AB = p[p[A + 1]];
    int BA = p[p[B + 0]];
    int AA = p[p[A + 0]];

    double u = myfade(x);
    double v = myfade(y);

    auto gradBB = mygrad(BB, x - 1, y - 1);
    auto gradAB = mygrad(AB, x, y - 1);
    auto gradBA = mygrad(BA, x - 1, y);
    auto gradAA = mygrad(AA, x, y);

    float res =
        mylerp(v,
            mylerp(u, gradAA, gradBA),
            mylerp(u, gradAB, gradBB)
        );

    return (res + .69f) / 1.483f;
}



std::optional<std::vector<std::string>> ReadFileLines(const fs::path& path) {
    //auto file = GetInFile(path);
    std::ifstream file(path, std::ios::binary);

    if (!file)
        return std::nullopt;

    std::vector<std::string> out;

    std::string line;
    while (std::getline(file, line)) {
        out.push_back(line);
    }

    return out;
}

int main(void) {

    fs::current_path("./data/tests");

    auto opt = ReadFileLines("perlin_values.txt");

    assert(opt && "file not found");

    auto&& values = opt.value();

    // the first line is the seed
    int next = 0;
    for (float y = -1.1f; y < 1.1f; y += .3f)
    {
        for (float x = -1.1f; x < 1.1f; x += .1f) {
            // Negative floats being truncated cause the mismatch
            // Uncomment to test the positive domain
            //if (x >= 0 && y >= 0)
            {
                float calc = PerlinNoise(x, y);
                float other = std::stof(values[next]);

                // not ideal because fp have epsilon
                //assert(calc == other)

                static constexpr float EPS = 0.0001f;
                assert((calc - EPS < other&& calc + EPS > other));
            }
            next++;
        }
    }

    return 0;
}
