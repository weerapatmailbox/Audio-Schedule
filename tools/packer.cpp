#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdint>
#include <algorithm>

namespace fs = std::filesystem;

// 32-byte rolling XOR key for payload obfuscation
static const uint8_t PAYLOAD_CIPHER_KEY[32] = {
    0x53, 0x6F, 0x66, 0x74, 0x6C, 0x75, 0x6E, 0x61, // "Softluna"
    0x72, 0x41, 0x75, 0x64, 0x69, 0x6F, 0x53, 0x63, // "rAudioSc"
    0x68, 0x65, 0x64, 0x75, 0x6C, 0x65, 0x32, 0x30, // "hedule20"
    0x32, 0x36, 0x50, 0x72, 0x6F, 0x74, 0x65, 0x63  // "26Protec"
};

inline void TransformPayload(uint8_t* data, size_t size, uint64_t streamPos) {
    for (size_t i = 0; i < size; ++i) {
        uint8_t keyByte = PAYLOAD_CIPHER_KEY[(streamPos + i) % 32];
        uint8_t salt = static_cast<uint8_t>((streamPos + i) * 3 + 7);
        data[i] ^= (keyByte ^ salt);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: packer <staging_directory> <output_payload_bin>" << std::endl;
        return 1;
    }

    fs::path stagingDir(argv[1]);
    fs::path outputFile(argv[2]);

    if (!fs::exists(stagingDir) || !fs::is_directory(stagingDir)) {
        std::cerr << "Error: Staging directory not found: " << stagingDir << std::endl;
        return 1;
    }

    std::vector<fs::path> files;
    for (const auto& entry : fs::recursive_directory_iterator(stagingDir)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path());
        }
    }

    if (outputFile.has_parent_path()) {
        fs::create_directories(outputFile.parent_path());
    }

    std::ofstream out(outputFile, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Error: Cannot open output file: " << outputFile << std::endl;
        return 1;
    }

    // Magic Header: "ASPE" (Audio Schedule Protected Encrypted payload)
    const char magic[4] = {'A', 'S', 'P', 'E'};
    out.write(magic, 4);

    // Total file count
    uint32_t fileCount = static_cast<uint32_t>(files.size());
    out.write(reinterpret_cast<const char*>(&fileCount), sizeof(fileCount));

    uint64_t totalBytes = 0;
    uint64_t cipherStreamPos = 0;

    for (const auto& filePath : files) {
        // Compute relative path
        fs::path rel = fs::relative(filePath, stagingDir);
        std::string relStr = rel.generic_u8string();

        uint16_t pathLen = static_cast<uint16_t>(relStr.size());
        out.write(reinterpret_cast<const char*>(&pathLen), sizeof(pathLen));

        // Encrypt relative path
        std::vector<uint8_t> encPath(relStr.begin(), relStr.end());
        TransformPayload(encPath.data(), encPath.size(), cipherStreamPos);
        cipherStreamPos += encPath.size();
        out.write(reinterpret_cast<const char*>(encPath.data()), encPath.size());

        uint64_t fileSize = fs::file_size(filePath);
        out.write(reinterpret_cast<const char*>(&fileSize), sizeof(fileSize));

        std::ifstream in(filePath, std::ios::binary);
        if (!in.is_open()) {
            std::cerr << "Error: Cannot open source file: " << filePath << std::endl;
            return 1;
        }

        std::vector<uint8_t> buffer(65536);
        uint64_t remaining = fileSize;
        while (remaining > 0) {
            size_t toRead = static_cast<size_t>(std::min<uint64_t>(remaining, buffer.size()));
            in.read(reinterpret_cast<char*>(buffer.data()), toRead);

            // Encrypt file chunk with rolling cipher stream position
            TransformPayload(buffer.data(), toRead, cipherStreamPos);
            cipherStreamPos += toRead;

            out.write(reinterpret_cast<const char*>(buffer.data()), toRead);
            remaining -= toRead;
        }

        totalBytes += fileSize;
    }

    out.close();
    std::cout << "Packed & Encrypted " << fileCount << " files (" << (totalBytes / (1024 * 1024.0)) << " MB) into " << outputFile.string() << std::endl;
    return 0;
}
