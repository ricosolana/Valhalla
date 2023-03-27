#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>

#include <robin_hood.h>

#include "IntrusiveList.h"

#include "ZDO.h"
#include "WorldManager.h"
#include "DataWriter.h"
#include "NetManager.h"
#include "VUtilsPhysics.h"
#include "DungeonManager.h"
#include "VUtils.h"

class Tests {
private:
    std::optional<BYTES_t> Test_ResourceBytes1(const fs::path& path) {
        std::ifstream file(path, std::ios::binary);

        if (!file)
            return std::nullopt;

        // https://www.reddit.com/r/cpp_questions/comments/m93tjb/comment/grkst7r/?utm_source=share&utm_medium=web2x&context=3

        return BYTES_t(
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>());        
    }

    std::optional<BYTES_t> Test_ResourceBytes2(const fs::path& path) {
        std::ifstream file(path, std::ios::binary);

        if (!file)
            return std::nullopt;

        file.unsetf(std::ios::skipws);

        std::streampos fileSize;

        file.seekg(0, std::ios::end);
        fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        BYTES_t vec;
        vec.resize(fileSize);
        file.read(reinterpret_cast<std::ifstream::char_type*>(&vec.front()), 
            fileSize);

        return vec;
    }

    std::optional<BYTES_t> Test_ResourceBytes3(const fs::path& path) {
        std::ifstream file(path, std::ios::binary);

        if (!file)
            return std::nullopt;

        BYTES_t vec;

        file.unsetf(std::ios::skipws);

        std::streampos fileSize;

        file.seekg(0, std::ios::end);
        fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        vec.reserve(fileSize);

        vec.insert(vec.begin(), std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

        return vec;
    }

    std::optional<BYTES_t> Test_ResourceBytes4(const fs::path& path) {
        FILE* file = fopen(path.string().c_str(), "rb");

        if (!file)
            return std::nullopt;

        BYTES_t vec;
        
        fseek(file, 0, SEEK_END);
        auto fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        // somehow avoid the zero-initialization
        vec.resize(fileSize);
        
        fread_s(vec.data(), vec.size(), 1, fileSize, file);

        fclose(file);

        return vec;
    }

    std::optional<std::string> Test_ResourceBytes5(const fs::path& path) {
        std::ifstream file(path, std::ios::binary);

        if (!file)
            return std::nullopt;

        file.unsetf(std::ios::skipws);
//        file.unsetf(std::ios::l)

        std::string result;

        file >> result;

        return result;
    }




    std::optional<std::list<std::string>> Test_ResourceLines1(const fs::path& path) {
        std::ifstream file(path, std::ios::in);

        if (!file)
            return std::nullopt;

        std::list<std::string> out;

        std::string line;
        while (std::getline(file, line)) {
            out.insert(out.end(), line);
        }

        return out;
    }

    std::optional<std::list<std::string>> Test_ResourceLines2(const fs::path& path) {
        FILE* file = fopen(path.string().c_str(), "r");

        if (!file)
            return std::nullopt;

        std::list<std::string> lines;

        char buf[512];
        while (fgets(buf, sizeof(buf) - 1, file)) {
            lines.push_back(buf);
        }

        return lines;
    }

    std::optional<std::list<std::string_view>> Test_ResourceLines3(const fs::path& path, bool blanks, std::string& out) {
        FILE* file = fopen(path.string().c_str(), "rb");

        if (!file)
            return std::nullopt;
        
        if (!fseek(file, 0, SEEK_END)) return std::nullopt;
        const int size = ftell(file); if (size == -1) return std::nullopt;
        if (!fseek(file, 0, SEEK_SET)) return std::nullopt;

        // somehow avoid the zero-initialization
        out.resize(size);

        char* data = out.data();
        
        if (fread(data, 1, size, file) != size && ferror(file))
            return std::nullopt;

        std::list<std::string_view> lines;

        int lineIdx = -1;
        int lineSize = 0;
        for (int i = 0; i < size; i++) {
            // if theres a newline at end of file, do not include

            // if at end of file and no newline at end

            lineSize = i - lineIdx - 1;
                        
            if (data[i] == '\n') {
                //if (lineSize || blanks) {
                if (lineSize) {
                    lines.push_back(std::string_view(data + lineIdx + 1, lineSize));
                }
                lineIdx = i;
            }
        }

        // this includes last line ONLY if it is not blank (has at least 1 character)
        if (lineIdx < size - 1) {
            lines.push_back(std::string_view(data + lineIdx + 1, size - lineIdx - 1));
        }

        return lines;
    }



    template<typename T = BYTES_t>
    std::optional<T> ReadFile(const fs::path& path) {
        //ScopedFile file(fopen(path.string().c_str(), "rb"));
        //
        //if (!file) return std::nullopt;
        //
        //if (!fseek(file, 0, SEEK_END)) return std::nullopt;
        //
        //auto size = ftell(file);
        //if (size == -1) return std::nullopt;
        //
        //if (!fseek(file, 0, SEEK_SET)) return std::nullopt;
        //
        //T result{};
        //
        //// somehow avoid the zero-initialization
        //result.resize(size);
        //
        //fread(result.data(), 1, size, file);
        //
        //return result;

        std::ifstream file(path, std::ios::binary);

        if (!file)
            return std::nullopt;

        file.unsetf(std::ios::skipws);

        std::streampos fileSize;

        file.seekg(0, std::ios::end);
        fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        T result{};
        result.resize(fileSize);
        file.read(reinterpret_cast<std::ifstream::char_type*>(&result.front()),
            fileSize);

        return result;
    }

    template<typename Iterable = std::vector<std::string>> requires
        (VUtils::Traits::is_iterable_v<Iterable>
            && std::is_same_v<typename Iterable::value_type, std::string>)
        std::optional<Iterable> ReadFileLines(const fs::path& path, bool includeBlanks = false) {
        auto opt = ReadFile<std::string>(path);
        if (!opt)
            return std::nullopt;

        auto size = opt.value().size();
        auto data = opt.value().data();

        Iterable lines;

        int lineIdx = -1;
        int lineSize = 0;
        for (int i = 0; i < size; i++) {
            lineSize = i - lineIdx - 1;

            if (data[i] == '\n') {
                if (lineSize || includeBlanks) {
                    lines.insert(lines.end(), Iterable::value_type(data + lineIdx + 1, lineSize));
                }
                lineIdx = i;
            }
        }

        // this includes last line ONLY if it is not blank (has at least 1 character)
        if (lineIdx < size - 1) {
            lines.insert(lines.end(), Iterable::value_type(data + lineIdx + 1, size - lineIdx - 1));
        }

        return lines;
    }

    template<typename Iterable = std::vector<std::string_view>> requires
        (VUtils::Traits::is_iterable_v<Iterable>
            && std::is_same_v<typename Iterable::value_type, std::string_view>)
        std::optional<Iterable> ReadFileLines(const fs::path& path, std::string& out, bool includeBlanks = false) {
        auto opt = ReadFile<std::string>(path);
        if (!opt)
            return std::nullopt;

        out = std::move(opt.value());
        auto size = out.size();
        auto data = out.data();

        Iterable lines;

        int lineIdx = -1;
        int lineSize = 0;
        for (int i = 0; i < size; i++) {
            lineSize = i - lineIdx - 1;

            if (data[i] == '\n') {
                if (lineSize || includeBlanks) {
                    lines.push_back(Iterable::value_type(data + lineIdx + 1, lineSize));
                }
                lineIdx = i;
            }
        }

        // this includes last line ONLY if it is not blank (has at least 1 character)
        if (lineIdx < size - 1) {
            lines.push_back(Iterable::value_type(data + lineIdx + 1, size - lineIdx - 1));
        }

        return lines;
    }





    bool Test_WriteFileBytes1(const fs::path& path, const BYTE_t* buf, int size) {
        std::ofstream file(path, std::ios::binary);

        if (!file)
            return false;

        std::copy(buf, buf + size,
            std::ostream_iterator<BYTE_t>(file));

        file.close();

        return true;
    }

    // this is pretty fast,
    //  only slower by the c method by only 5%
    //  and this is tons safer and more modern than the c method...
    //  I might settle on this
    bool Test_WriteFileBytes2(const fs::path& path, const BYTE_t* buf, int size) {
        std::ofstream file(path, std::ios::binary);

        if (!file)
            return false;

        file.write(reinterpret_cast<const char*>(buf), size);
    }

    bool Test_WriteFileBytes3(const fs::path& path, const BYTE_t* buf, int size) {
        FILE* file = fopen(path.string().c_str(), "wb");

        if (!file)
            return false;

        auto sizeWritten = fwrite(buf, 1, size, file);
        if (sizeWritten != size) {
            fclose(file);
            LOG(ERROR) << "File write error";
            return false;
        }

        fclose(file);

        return true;
    }




    template<typename Iterable> requires
        (VUtils::Traits::is_iterable_v<Iterable>
            && std::is_same_v<typename Iterable::value_type, std::string>)
        bool Test_WriteFileLines0(const fs::path& path, const Iterable& in)
    {
        std::ofstream file(path, std::ios::binary);

        if (!file)
            return false;

        for (auto&& str : in) {
            //file << str << "\n";
            file.write(str.c_str(), str.size());
            file.write("\n", 1);
        }

        file.close();

        return true;
    }

    template<typename Iterable> requires
        (VUtils::Traits::is_iterable_v<Iterable>
            && std::is_same_v<typename Iterable::value_type, std::string>)
        bool Test_WriteFileLines1(const fs::path& path, const Iterable& in) 
    {
        std::ofstream file(path, std::ios::binary);

        if (!file)
            return false;

        for (auto&& str : in) {
            file << str << "\n";
        }

        file.close();

        return true;
    }

    template<typename Iterable> requires
        (VUtils::Traits::is_iterable_v<Iterable>
            && std::is_same_v<typename Iterable::value_type, std::string>)
        bool Test_WriteFileLines2(const fs::path& path, const Iterable& in) 
    {
        FILE* file = fopen(path.string().c_str(), "wb");

        if (!file) return false;

        for (auto&& s : in) {
            fwrite(s.data(), 1, s.size(), file);
            fwrite("\n", 1, sizeof("\n"), file);
        }

        fclose(file);

        return true;
    }

    template<typename Iterable> requires
        (VUtils::Traits::is_iterable_v<Iterable>
            && std::is_same_v<typename Iterable::value_type, std::string>)
        bool Test_WriteFileLines3(const fs::path& path, const Iterable& in)
    {
        FILE* file = fopen(path.string().c_str(), "wb");

        if (!file) return false;

        for (auto&& s : in) {
            std::string val = s + "\n";
            fwrite(val.data(), 1, val.size(), file);
        }

        fclose(file);

        return true;
    }




public:
    using StringList = PCSX::Intrusive::List<std::string>;

    void TestIntrusive() {
        sol::state lua;
        lua.open_libraries();

        lua.new_usertype<StringList>(//"StringList",
            //sol::meta_function::length, &StringList::size
            [](const StringList& rl) {
                return sol::as_container(rl); // Required for sol to treat Intrusive Lists as a container
            }
        );

        StringList intrusive;
        intrusive.push_back(StringList::Node::Node().);
        //StringList copy = intrusive;

        lua["intrusive"] = &intrusive;

        const auto& code = R"(
            for i = 1, #intrusive do
                --print(intrusive[i])
            end
        )";

        lua.script(code);

        return;
    }


    void RunTests() {
        fs::current_path("./data/tests/");

        //Test_ZStdCompressorDecompressor();

        TestIntrusive();

        //Tests().Test_FileWriteLines();

        //Tests().Test_ResourceReadBytes();
        //Tests().Test_ResourceLines();

        //Tests().Test_FileWriteBytes();

        //Tests().Test_ParentChildTransforms();

        //Tests().Test_DungeonGenerator();

        //Tests().Test_RectInsideRect();
        //Tests().Test_RectOverlap();

        //Tests().Test_LinesIntersect();

        //Tests().Test_ParentChildTransforms();
        //Tests::Test_QuaternionLook();

        //Tests::Test_PeerLuaConnect();
        //Tests::Test_DataBuffer();
        //Tests::Test_World();
        //Tests().Test_ZDO();
        //Tests::Test_ResourceReadWrite();
        //Tests::Test_Random();
        //Tests::Test_Perlin();

        LOG(INFO) << "All tests passed!";
    }


    void Test_ZStdCompressorDecompressor() {

        auto dict = *VUtils::Resource::ReadFile<BYTES_t>("dict/small");

        ZStdCompressor compressor(dict);
        ZStdDecompressor decompressor(dict);

        auto declaration = *VUtils::Resource::ReadFile<BYTES_t>("dict/declaration.txt");

        auto c_opt = compressor.Compress(declaration);
        auto d_opt = decompressor.Decompress(*c_opt);

        assert(declaration == *d_opt);
    }







    //2023-03-15 00:57:55,531 INFO [default] Starting trials in 3s
    //2023-03-15 00:57:58,546 INFO [default] Starting ResourceBytes1...
    //2023-03-15 00:58:10,911 INFO [default] Test_ResourceBytes1: 12.357s
    //2023-03-15 00:58:10,915 INFO [default] Starting ResourceBytes2...
    //2023-03-15 00:58:12,174 INFO [default] Test_ResourceBytes2: 1.258s
    //2023-03-15 00:58:12,174 INFO [default] Starting ResourceBytes3...
    //2023-03-15 00:58:21,588 INFO [default] Test_ResourceBytes3: 9.412s
    //2023-03-15 00:58:21,588 INFO [default] Starting ResourceBytes4
    //2023-03-15 00:58:22,319 INFO [default] Test_ResourceBytes4: 0.73s
    //2023-03-15 00:58:22,319 INFO [default] All tests passed!

    // the c methods are the fastest, faster than the 'recommended' c++ methods more than 15x faster
    //  so dumb...
    // there are more limitations, but this is still insane...

    // ive not even tried removing vector resize from the equation or tried asm...
    // this was tested in release mode several times, with all results being similar each new running instance
    // the #4 method is the fastest, but it utilizes c FILE/fopen/fread, which is considered unsafe/not modern
    // the next option is to use the ifstream method, which is about 1.72x slower than the c way
    //  the c way I am utilizing is not even the best because: 
    //  - still using vector resize
    //  - fread only supports reading ~2gb files
    //  for these reasons, I should use fread64 or the equivalent to read larger files quickly
    //  I am not sure why ifstream/iterator is still 16x slower than fopen/fread

    // maybe the fopen/fread method is best for situations where files <2GB are being opened because
    //  not many files go over that. It seems best for my use-cases, and its speedy af

    void Test_ResourceReadBytes() {
        static constexpr int TRIALS = 10;

        LOG(INFO) << "Starting trials in 3s";
        std::this_thread::sleep_for(3s);

        {
            LOG(INFO) << "Starting ResourceBytes1...";

            auto start(std::chrono::steady_clock::now());
            for (int i = 0; i < TRIALS; i++) {
                auto opt = Test_ResourceBytes1("worlds/ClanWarsS01.db");
                if (opt)
                    printf(""); // maybe prevents optimizizing away opt
            }
            auto now(std::chrono::steady_clock::now());
            LOG(INFO) << "Test_ResourceBytes1: " << ((float)duration_cast<milliseconds>(now - start).count())/1000.f << "s";
        }

        {
            LOG(INFO) << "Starting ResourceBytes2...";

            auto start(std::chrono::steady_clock::now());
            for (int i = 0; i < TRIALS; i++) {
                auto opt = Test_ResourceBytes2("worlds/ClanWarsS01.db");
                if (opt)
                    printf(""); // maybe prevents optimizizing away opt
            }
            auto now(std::chrono::steady_clock::now());
            LOG(INFO) << "Test_ResourceBytes2: " << ((float)duration_cast<milliseconds>(now - start).count()) / 1000.f << "s";
        }

        {
            LOG(INFO) << "Starting ResourceBytes3...";

            auto start(std::chrono::steady_clock::now());
            for (int i = 0; i < TRIALS; i++) {
                auto opt = Test_ResourceBytes3("worlds/ClanWarsS01.db");
                if (opt)
                    printf(""); // maybe prevents optimizizing away opt
            }
            auto now(std::chrono::steady_clock::now());
            LOG(INFO) << "Test_ResourceBytes3: " << ((float)duration_cast<milliseconds>(now - start).count()) / 1000.f << "s";
        }

        {
            LOG(INFO) << "Starting ResourceBytes4";

            auto start(std::chrono::steady_clock::now());
            for (int i = 0; i < TRIALS; i++) {
                auto opt = Test_ResourceBytes4("worlds/ClanWarsS01.db");
                if (opt)
                    printf(""); // maybe prevents optimizizing away opt
            }
            auto now(std::chrono::steady_clock::now());
            LOG(INFO) << "Test_ResourceBytes4: " << ((float)duration_cast<milliseconds>(now - start).count()) / 1000.f << "s";
        }

        {
            LOG(INFO) << "Starting ResourceBytes5";

            auto start(std::chrono::steady_clock::now());
            for (int i = 0; i < TRIALS; i++) {
                auto opt = Test_ResourceBytes5("worlds/ClanWarsS01.db");
                if (opt)
                    printf(""); // maybe prevents optimizizing away opt
            }
            auto now(std::chrono::steady_clock::now());
            LOG(INFO) << "Test_ResourceBytes5: " << ((float)duration_cast<milliseconds>(now - start).count()) / 1000.f << "s";
        }
    }



    void Test_ResourceLines() {
        static constexpr int TRIALS = 1;

        //std::string ss;
        //Test_ResourceLines3("smallfile.txt", true, ss);
        //printf("%c", ss[0]);



        LOG(INFO) << "Starting trials in 3s";
        std::this_thread::sleep_for(3s);

        /*
        {
            LOG(INFO) << "Starting ResourceLines1...";

            auto start(std::chrono::steady_clock::now());
            for (int i = 0; i < TRIALS; i++) {
                auto opt = Test_ResourceLines1("bigfile.txt");
                if (opt)
                    printf(""); // maybe prevents optimizizing away opt
            }
            auto now(std::chrono::steady_clock::now());
            LOG(INFO) << "Test_ResourceLines1: " << ((float)duration_cast<milliseconds>(now - start).count()) / 1000.f << "s";
        }

        {
            LOG(INFO) << "Starting ResourceLines2...";

            auto start(std::chrono::steady_clock::now());
            for (int i = 0; i < TRIALS; i++) {
                auto opt = Test_ResourceLines2("bigfile.txt");
                if (opt)
                    printf(""); // maybe prevents optimizizing away opt
            }
            auto now(std::chrono::steady_clock::now());
            LOG(INFO) << "Test_ResourceLines2: " << ((float)duration_cast<milliseconds>(now - start).count()) / 1000.f << "s";
        }
        */

        //{
        //    LOG(INFO) << "Starting ResourceLines3...";
        //
        //    auto start(std::chrono::steady_clock::now());
        //    for (int i = 0; i < TRIALS; i++) {
        //        std::string out;
        //        auto opt = Test_ResourceLines3("bigfile.txt", false, out);
        //        if (opt)
        //            printf(""); // maybe prevents optimizizing away opt
        //    }
        //    auto now(std::chrono::steady_clock::now());
        //    LOG(INFO) << "Test_ResourceLines3: " << ((float)duration_cast<milliseconds>(now - start).count()) / 1000.f << "s";
        //}

        {
            LOG(INFO) << "Starting ReadFileLines...";

            auto start(std::chrono::steady_clock::now());
            for (int i = 0; i < TRIALS; i++) {
                std::string out;
                auto opt = ReadFileLines("bigfile.txt", out);
                if (opt)
                    printf(""); // maybe prevents optimizizing away opt
            }
            auto now(std::chrono::steady_clock::now());
            LOG(INFO) << "Test_ResourceLines2: " << ((float)duration_cast<milliseconds>(now - start).count()) / 1000.f << "s";
        }
    }

    void Test_FileWriteBytes() {
        static constexpr int TRIALS = 5;

        LOG(INFO) << "Generating random file data...";
        auto val = VUtils::Random::GenerateAlphaNum(30000000);

        LOG(INFO) << "Starting trials in 3s";
        std::this_thread::sleep_for(3s);

        {
            LOG(INFO) << "Starting FileWriteBytes1...";

            auto start(std::chrono::steady_clock::now());
            for (int i = 0; i < TRIALS; i++) {
                auto opt = Test_WriteFileBytes1("writelargefile.txt", 
                    reinterpret_cast<BYTE_t*>(val.data()), val.size());
                if (opt)
                    printf(""); // maybe prevents optimizizing away opt
            }
            auto now(std::chrono::steady_clock::now());
            LOG(INFO) << "Test_WriteFileBytes1: " << ((float)duration_cast<milliseconds>(now - start).count()) / 1000.f << "s";
        }

        {
            LOG(INFO) << "Starting FileWriteBytes2...";

            auto start(std::chrono::steady_clock::now());
            for (int i = 0; i < TRIALS; i++) {
                auto opt = Test_WriteFileBytes2("writelargefile.txt",
                    reinterpret_cast<BYTE_t*>(val.data()), val.size());
                if (opt)
                    printf(""); // maybe prevents optimizizing away opt
            }
            auto now(std::chrono::steady_clock::now());
            LOG(INFO) << "Test_WriteFileBytes2: " << ((float)duration_cast<milliseconds>(now - start).count()) / 1000.f << "s";
        }

        {
            LOG(INFO) << "Starting FileWriteBytes3...";

            auto start(std::chrono::steady_clock::now());
            for (int i = 0; i < TRIALS; i++) {
                auto opt = Test_WriteFileBytes3("writelargefile.txt",
                    reinterpret_cast<BYTE_t*>(val.data()), val.size());
                if (opt)
                    printf(""); // maybe prevents optimizizing away opt
            }
            auto now(std::chrono::steady_clock::now());
            LOG(INFO) << "Test_WriteFileBytes3: " << ((float)duration_cast<milliseconds>(now - start).count()) / 1000.f << "s";
        }
    }

    void Test_FileWriteLines() {
        static constexpr int TRIALS = 2000;

        LOG(INFO) << "Generating random file data...";
        std::list<std::string> val;
        for (int i = 0; i < 1000; i++) {
            val.push_back(VUtils::Random::GenerateAlphaNum(300 + (i%100)));
        }

        LOG(INFO) << "Starting trials in 3s";
        std::this_thread::sleep_for(3s);

        {
            LOG(INFO) << "Starting FileWriteLines0...";

            auto start(std::chrono::steady_clock::now());
            for (int i = 0; i < TRIALS; i++) {
                auto opt = Test_WriteFileLines0("writefilelines.txt", val);
                if (opt)
                    printf(""); // maybe prevents optimizizing away opt
            }
            auto now(std::chrono::steady_clock::now());
            LOG(INFO) << "FileWriteLines0: " << ((float)duration_cast<milliseconds>(now - start).count()) / 1000.f << "s";
        }

        {
            LOG(INFO) << "Starting FileWriteLines1...";

            auto start(std::chrono::steady_clock::now());
            for (int i = 0; i < TRIALS; i++) {
                auto opt = Test_WriteFileLines1("writefilelines.txt", val);
                if (opt)
                    printf(""); // maybe prevents optimizizing away opt
            }
            auto now(std::chrono::steady_clock::now());
            LOG(INFO) << "FileWriteLines1: " << ((float)duration_cast<milliseconds>(now - start).count()) / 1000.f << "s";
        }

        {
            LOG(INFO) << "Starting FileWriteLines2...";

            auto start(std::chrono::steady_clock::now());
            for (int i = 0; i < TRIALS; i++) {
                auto opt = Test_WriteFileLines2("writefilelines.txt", val);
                if (opt)
                    printf(""); // maybe prevents optimizizing away opt
            }
            auto now(std::chrono::steady_clock::now());
            LOG(INFO) << "FileWriteLines2: " << ((float)duration_cast<milliseconds>(now - start).count()) / 1000.f << "s";
        }

        {
            LOG(INFO) << "Starting FileWriteLines3...";

            auto start(std::chrono::steady_clock::now());
            for (int i = 0; i < TRIALS; i++) {
                auto opt = Test_WriteFileLines3("writefilelines.txt", val);
                if (opt)
                    printf(""); // maybe prevents optimizizing away opt
            }
            auto now(std::chrono::steady_clock::now());
            LOG(INFO) << "FileWriteLines3: " << ((float)duration_cast<milliseconds>(now - start).count()) / 1000.f << "s";
        }
    }



    void Test_DungeonGenerator() {
        Valhalla()->LoadFiles();

        PrefabManager()->Init();
        DungeonManager()->Init();

        //DungeonManager()->GetDungeon(
        //    //VUtils::String::GetStableHashCode("DG_Cave")
        //    VUtils::String::GetStableHashCode("DG_SunkenCrypt")
        //)->Generate(Vector3(130, 234, 2423), Quaternion::IDENTITY);

        //DungeonManager()->GetDungeon(
        //    VUtils::String::GetStableHashCode("DG_SunkenCrypt")
        //)->Generate(Vector3(2513.1, 5031.8, -4212.3), Quaternion(0.0, -0.2, 0.0, -1.0), 1372687413);
    }

    void Test_LinesIntersect() {
        {
            Vector2 a(.5f, -.5f);
            Vector2 b(.5f, .5f);
            Vector2 c(.3901f, .6901f);
            Vector2 d(.9098f, .3901f);

            //assert(!VUtils::Physics::LinesIntersect(
            //    a, b, c, d
            //));
        }
    }

    void Test_RectInsideRect() {
        {
            Vector3 size(100, 0, 50);

            Vector3 pos1(0, 0, 0);
            Vector3 pos2(51, 0, 0);

            auto rot(Quaternion::Euler(0, 0, 0));

            assert(!VUtils::Physics::PointInsideRect(
                size, pos1, rot,
                pos2
            ));
        }

        {
            Vector3 size(100, 0, 50);

            Vector3 pos1(0, 0, 0);
            Vector3 pos2(49, 0, 0);

            auto rot(Quaternion::Euler(0, 0, 0));

            assert(VUtils::Physics::PointInsideRect(
                size, pos1, rot,
                pos2
            ));
        }

        {
            Vector3 size(100, 0, 50);

            Vector3 pos1(0, 0, 0);
            Vector3 pos2(32, 0, 18);

            auto rot(Quaternion::Euler(0, 30, 0));

            assert(!VUtils::Physics::PointInsideRect(
                size, pos1, rot,
                pos2
            ));
        }



        {
            Vector3 size1(100, 0, 50);
            Vector3 size2(100, 0, 50);

            Vector3 pos1(0, 0, 0);
            Vector3 pos2(100, 0, 0);

            auto rot1(Quaternion::Euler(0, 0, 0));
            auto rot2(Quaternion::Euler(0, 0, 0));

            assert(!VUtils::Physics::RectInsideRect(
                size1, pos1, rot1,
                size2, pos2, rot2
            ));
        }

        {
            Vector3 size1(100, 0, 50);
            Vector3 size2(90, 0, 40);

            Vector3 pos1(0, 0, 0);
            Vector3 pos2(0, 0, 0);

            auto rot1(Quaternion::Euler(0, 0, 0));
            auto rot2(Quaternion::Euler(0, 0, 0));

            assert(VUtils::Physics::RectInsideRect(
                size1, pos1, rot1,
                size2, pos2, rot2
            ));
        }

        {
            Vector3 size1(1.f, 0, 1.f);
            Vector3 size2(.9f, 0, .9f);

            Vector3 pos1(0, 0, 0);
            Vector3 pos2(0, 0, 0);

            auto rot1(Quaternion::Euler(0, 0, 0));
            auto rot2(Quaternion::Euler(0, 45, 0));

            assert(!VUtils::Physics::RectInsideRect(
                size1, pos1, rot1,
                size2, pos2, rot2
            ));
        }

        {
            Vector3 size1(1.f, 0, 1.f);
            Vector3 size2(.6f, 0, .6f);

            Vector3 pos1(0, 0, 0);
            Vector3 pos2(0, 0, 0);

            auto rot1(Quaternion::Euler(0, 0, 0));
            auto rot2(Quaternion::Euler(0, 45.f, 0));

            assert(VUtils::Physics::RectInsideRect(
                size1, pos1, rot1,
                size2, pos2, rot2
            ));
        }

        {
            Vector3 size1(1, 0, 1);
            Vector3 size2(.6f, 0, .6f);

            Vector3 pos1(0, 0, 0);
            Vector3 pos2(0, 1, 0);

            auto rot1(Quaternion::Euler(0, 0, 0));
            auto rot2(Quaternion::Euler(0, 45, 0));

            assert(!VUtils::Physics::RectInsideRect(
                size1, pos1, rot1,
                size2, pos2, rot2
            ));
        }
    }

    void Test_RectOverlap() {
        // Rectangles touching side-to-side will always be considered overlapping
        //{
        //    Vector3 size1(100, 0, 50);
        //    Vector3 size2(100, 0, 50);
        //
        //    Vector3 pos1(0, 0, 0);
        //    Vector3 pos2(100, 0, 0);
        //
        //    auto rot1(Quaternion::Euler(0, 15, 0));
        //    auto rot2(Quaternion::Euler(0, 0, 0));
        //
        //    assert(!VUtils::Physics::RectOverlapRect(
        //        size1, pos1, rot1,
        //        size2, pos2, rot2
        //    ));
        //}

        {
            Vector3 size1(1.f, 0, 1.f);
            Vector3 size2(.7f, 0, .7f);

            Vector3 pos1(0, 0, 0);
            Vector3 pos2(0, 0, 0);

            auto rot1(Quaternion::Euler(0, 0, 0));
            auto rot2(Quaternion::Euler(0, 30, 0));

            assert(VUtils::Physics::RectOverlapRect(
                size1, pos1, rot1,
                size2, pos2, rot2
            ));
        }

        {
            Vector3 size1(1.f, 0, 1.f);
            Vector3 size2(.6f, 0, .6f);

            Vector3 pos1(0, 0, 0);
            Vector3 pos2(.8f, 0, .8f);

            auto rot1(Quaternion::Euler(0, 0, 0));
            auto rot2(Quaternion::Euler(0, 30.f, 0));

            assert(!VUtils::Physics::RectOverlapRect(
                size1, pos1, rot1,
                size2, pos2, rot2
            ));
        }

        {
            Vector3 size1(1.f, 0, 1);
            Vector3 size2(.6f, 0, .6f);

            Vector3 pos1(.15f, 0, .15f);
            Vector3 pos2(.8f, 0, .8f);

            auto rot1(Quaternion::Euler(0, 45, 0));
            auto rot2(Quaternion::Euler(0, 45, 0));

            assert(!VUtils::Physics::RectOverlapRect(
                size1, pos1, rot1,
                size2, pos2, rot2
            ));
        }

        {
            Vector3 size1(1.f, 0, 1);
            Vector3 size2(.6f, 0, .6f);

            Vector3 pos1(-.326f, 0, -.474f);
            Vector3 pos2(.6f, 0, -.04f);

            auto rot1(Quaternion::Euler(0, 210, 0));
            auto rot2(Quaternion::Euler(0, 60, 0));

            assert(!VUtils::Physics::RectOverlapRect(
                size1, pos1, rot1,
                size2, pos2, rot2
            ));
        }


    }

    //void Test_LineOverlap() {
    //
    //}

    void Test_ParentChildTransforms() {
        {
            // Dummy child local transforms
            Vector3 childLocalPos(1, 1, 1);
            Quaternion childLocalRot(Quaternion::Euler(10, 75, 10));

            // Dummy parent transforms
            Vector3 parentPos(4, 5, 4);
            Quaternion parentRot(Quaternion::Euler(20, 90, 30));



            // Unity-calculate values based on the above
            //  World transforms of child
            Vector3 expectChildPos(5.406901f, 5.941624f, 3.633975f);
            Quaternion expectChildRot(.1371928f, .2958279f, .9315588f, .1608175f);

            //Quaternion calcChildRot = parentRot * childLocalRot;
            //Vector3 pointOnRot = (calcChildRot * Vector3::FORWARD).Normalized() * childLocalPos.Magnitude();

            //Vector3 calcChildPos = pointOnRot + parentPos;

            auto global = VUtils::Physics::LocalToGlobal(childLocalPos, childLocalRot,
                parentPos, parentRot
            );
        }

        {
            // Dummy child local transforms
            Vector3 childLocalPos(12, 0, 0);
            Quaternion childLocalRot(Quaternion::Euler(0, 10, 0));

            // Dummy parent transforms
            Vector3 parentPos(-182, 52, -52);
            Quaternion parentRot(Quaternion::Euler(0, 24, 0));



            // Unity-calculate values based on the above
            //  World transforms of child
            Vector3 expectChildPos(-171.0375f, 52, -56.88084f);
            Quaternion expectChildRot(Quaternion::Euler(0, 34, 0));

            //Quaternion calcChildRot = parentRot * childLocalRot;
            //Vector3 pointOnRot = (calcChildRot * Vector3::FORWARD).Normalized() * childLocalPos.Magnitude();

            //Vector3 calcChildPos = pointOnRot + parentPos;

            auto global = VUtils::Physics::LocalToGlobal(childLocalPos, childLocalRot,
                parentPos, parentRot
            );

            if (true) {}
                
        }

        {
            // Dummy child local transforms
            Vector3 childLocalPos(12, 0, 0);
            Quaternion childLocalRot(Quaternion::Euler(0, -12, 0));

            // Dummy parent transforms
            Vector3 parentPos(-182, 52, -52);
            Quaternion parentRot(Quaternion::Euler(0, 24, 0));



            // Unity-calculate values based on the above
            //  World transforms of child
            Vector3 expectChildPos(-171.0375f, 52, -56.88084f);
            Quaternion expectChildRot(Quaternion::Euler(0, 12, 0));

            //Quaternion calcChildRot = parentRot * childLocalRot;
            //Vector3 pointOnRot = (calcChildRot * Vector3::FORWARD).Normalized() * childLocalPos.Magnitude();

            //Vector3 calcChildPos = pointOnRot + parentPos;

            auto global = VUtils::Physics::LocalToGlobal(childLocalPos, childLocalRot,
                parentPos, parentRot
            );

            if (true) {}

        }
    }

    void Test_QuaternionLook() {
        auto opt = VUtils::Resource::ReadFile<std::vector<std::string>>("lookrotation_values.txt");

        assert(opt && "file not found");

        auto&& values = opt.value();
        int index = 0;

        for (float z = -1; z < 1; z += .05f)
        {
            for (float y = -1; y < 1; y += .05f)
            {
                for (float x = -1; x < 1; x += .05f)
                {
                    auto vec = Vector3(x, y, z);
                    if (vec.Magnitude() <= 0.0001f)
                        continue;

                    auto calc = Quaternion::LookRotation(Vector3(x, y, z));
                    auto expect = Quaternion(
                        std::stof(values[index + 0]),
                        std::stof(values[index + 1]),
                        std::stof(values[index + 2]),
                        std::stof(values[index + 3])
                    );

                    index += 4;

                    //if (y < 180 || x < 60)
                        //continue;

                    static constexpr float EPS = 0.0001f;
                    assert((calc.x - EPS < expect.x&& calc.x + EPS > expect.x));
                    assert((calc.y - EPS < expect.y&& calc.y + EPS > expect.y));
                    assert((calc.z - EPS < expect.z&& calc.z + EPS > expect.z));
                    assert((calc.w - EPS < expect.w&& calc.w + EPS > expect.w));
                }
            }
        }
    }

    void Test_QuaternionEuler() {
        auto opt = VUtils::Resource::ReadFile<std::vector<std::string>>("euler_values.txt");

        assert(opt && "file not found");

        auto&& values = opt.value();
        int index = 0;

        for (float z = 0; z < 360; z += 45)
        {
            for (float y = 0; y < 360; y += 30)
            {
                for (float x = 0; x < 360; x += 10)
                {
                    auto calc = Quaternion::Euler(x, y, z);
                    auto expect = Quaternion(
                        std::stof(values[index + 0]),
                        std::stof(values[index + 1]),
                        std::stof(values[index + 2]),
                        std::stof(values[index + 3])
                    );

                    index += 4;

                    //if (y < 180 || x < 60)
                        //continue;

                    static constexpr float EPS = 0.0001f;
                    assert((calc.x - EPS < expect.x && calc.x + EPS > expect.x));
                    assert((calc.y - EPS < expect.y && calc.y + EPS > expect.y));
                    assert((calc.z - EPS < expect.z && calc.z + EPS > expect.z));
                    assert((calc.w - EPS < expect.w && calc.w + EPS > expect.w));

                    
                }
            }
        }
    }
    
    void Test_PeerLuaConnect() {
        //Peer ref(nullptr, 123456789, "eikthyr", Vector3::ZERO);
        //
        //Peer* peer = &ref;
        //
        //// tests a fake player
        //ModManager()->Init();
        //
        //ModManager()->CallEvent(VUtils::String::GetStableHashCode("PeerInfo"), peer);
    }

    void Test_DataBuffer() {
        //constexpr int has = is_container<std::vector<int>>::value;
        //constexpr int has = is_container<int>::value;
        //
        //constexpr int has = is_container_of<int, int>::value;
        //constexpr int has1 = is_container_of<std::vector<int>, int>::value;
        //
        //constexpr int has2 = has_value_type<std::vector<int>, int>::value;

        //std::indirectly_readable_traits<std::vector<int>>::

        //constexpr int i = has_value_type<int>::value;
        //constexpr int i = has_value_type<std::vector<int>>::value;

        //std::iter_value_t<int>

        BYTES_t bytes;
        DataWriter writer(bytes);

        //int count = 1114111;
        //int count = 111111;

        int count = 0xFFF1;

        writer.Write((char16_t)count);

        assert(count == DataReader(bytes).ReadChar());

    }

    void Test_World() {
        WorldManager()->BackupFileWorldDB("world");

        auto world = WorldManager()->GetWorld("privUWorld");
        WorldManager()->LoadFileWorldDB("02129");
    }

    void Test_ZDO() {

        // Unique hash tests
        {
            {
#ifdef RUN_TESTS
                const HASH_t hash1 = 14516234;
                assert(ZDO::FromShiftHash<float>(ZDO::ToShiftHash<float>(hash1)) == hash1);

                const HASH_t hash2 = 56827231;
                assert(ZDO::FromShiftHash<int>(ZDO::ToShiftHash<int>(hash2)) == hash2);

                const HASH_t hash3 = 906582783;
                assert(ZDO::FromShiftHash<std::string>(ZDO::ToShiftHash<std::string>(hash3)) == hash3);
#endif
            }



        }

        // Valheim-sourced Load tests
        {
            auto opt = VUtils::Resource::ReadFile<BYTES_t>("zdo.sav");
            assert(opt);

            ZDO zdo;
            BYTES_t bytes;
            DataReader pkg(opt.value());
            zdo.Load(pkg, VConstants::WORLD);

            assert(zdo.GetFloat("health", 0) == 3.1415926535f);
            assert(zdo.GetInt("weight", 0) == 435);
            assert(zdo.GetInt("slot", 0) == 3);
            assert(zdo.GetString("name", "") == "byeorgssen");
            assert(zdo.GetString("faction", "") == "player");
            assert(zdo.GetInt("uid", 0) == 189341389);
        }

        // Set/Get tests
        {
            ZDO zdo;

            zdo.Set("health", 3.1415926535f);
            zdo.Set("weight", 435);
            zdo.Set("slot", 3);
            zdo.Set("name", "byeorgssen");
            zdo.Set("faction", "player");
            zdo.Set("uid", 189341389);

            assert(zdo.GetFloat("health", 0) == 3.1415926535f);
            assert(zdo.GetInt("weight", 0) == 435);
            assert(zdo.GetInt("slot", 0) == 3);
            assert(zdo.GetString("name", "") == "byeorgssen");
            assert(zdo.GetString("faction", "") == "player");
            assert(zdo.GetInt("uid", 0) == 189341389);
        }

        // Save/Load tests
        {
            ZDO zdo;

            //zdo.Set("health", 3.1415926535f);
            //zdo.Set("weight", 435);
            //zdo.Set("slot", 3);
            //zdo.Set("name", "byeorgssen");
            zdo.Set("faction", "player");
            //zdo.Set("uid", 189341389);
            
            BYTES_t bytes;
            {
                DataWriter pkg(bytes);
                zdo.Save(pkg);
            }

            ZDO zdo2;
            DataReader reader(bytes);
            zdo2.Load(reader, VConstants::WORLD);

            //assert(zdo2.GetFloat("health", 0) == 3.1415926535f);
            //assert(zdo2.GetInt("weight", 0) == 435);
            //assert(zdo2.GetInt("slot", 0) == 3);
            //assert(zdo2.GetString("name", "") == "byeorgssen");
            assert(zdo2.GetString("faction", "") == "player");
            //assert(zdo2.GetInt("uid", 0) == 189341389);
        }

        // Serialize/Deserialize tests
        {
            ZDO zdo;

            zdo.Set("health", 3.1415926535f);
            zdo.Set("weight", 435);
            zdo.Set("slot", 3);
            zdo.Set("name", "byeorgssen");
            zdo.Set("faction", "player");
            zdo.Set("uid", 189341389);

            BYTES_t bytes;
            DataWriter writer(bytes);
            zdo.Serialize(writer);
            writer.SetPos(0);

            DataReader reader(bytes);
            ZDO zdo2;
            zdo2.Deserialize(reader);

            assert(zdo2.GetFloat("health", 0) == 3.1415926535f);
            assert(zdo2.GetInt("weight", 0) == 435);
            assert(zdo2.GetInt("slot", 0) == 3);
            assert(zdo2.GetString("name", "") == "byeorgssen");
            assert(zdo2.GetString("faction", "") == "player");
            assert(zdo2.GetInt("uid", 0) == 189341389);
        }
    }

    void Test_ResourceReadWrite() {
        std::vector<std::string> linesToWrite;
        int itr = rand();
        for (int i = 0; i < itr; ++i) linesToWrite.push_back(VUtils::Random::GenerateAlphaNum((rand() % 10) + 10));
        VUtils::Resource::WriteFile("test_write_lines.txt", linesToWrite);

        // now read file
        auto opt = VUtils::Resource::ReadFile<std::vector<std::string>>("test_write_lines.txt");
        assert(opt && "file not found");
        assert(opt.value() == linesToWrite);
    }

    void Test_Random() {
        // The plan is to read in the Unity random file with many values, then compare against it

        // read in integers separated by a \n
        auto opt = VUtils::Resource::ReadFile<std::vector<std::string>>("random_values.txt");

        assert(opt && "file not found");

        auto&& values = opt.value();

        // the first line is the seed
        VUtils::Random::State state(std::stoi(values[0]));

        for (int i = 0; i < 100; ++i) {
            assert(state.Range(std::numeric_limits<int>::min(), std::numeric_limits<int>::max())
                == std::stoi(values[i + 1]));
        }
    }

    void Test_Perlin() {
        // brute force tested
        //VUtils::Math::BruteForcePerlinNoise(-1.1f, -1.1f, 0.597924829f);



        auto opt = VUtils::Resource::ReadFile<std::vector<std::string>>("perlin_values.txt");

        assert(opt && "file not found");

        auto&& values = opt.value();

        // the first line is the seed
        int next = 0;
        for (float y = -1.1f; y < 1.1f; y += .3f)
        {
            for (float x = -1.1f; x < 1.1f; x += .1f) {
                // Negative floats being truncated cause the mismatch
                if (x >= 0 && y >= 0)
                {
                    float calc = VUtils::Math::PerlinNoise(x, y);
                    float other = std::stof(values[next]);
                    // not ideal because floating point has en epsilon diff
                    //assert(calc == other)

                    static constexpr float EPS = 0.0001f;
                    assert((calc - EPS < other && calc + EPS > other));
                }
                next++;
            }
        }
    }
};
