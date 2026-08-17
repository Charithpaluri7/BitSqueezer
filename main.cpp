#include <iostream>
#include <fstream>
#include <queue>
#include <vector>
#include <string>
#include <cstring>
#include <memory>
#include <array>
#include <cstdint>
#include <iomanip>

using namespace std;

struct Node;
using NodePtr = shared_ptr<Node>;

struct Node {
    uint8_t byte;
    uint64_t freq;
    NodePtr left, right;

    Node(uint8_t b, uint64_t f) : byte(b), freq(f) {}

    Node(NodePtr l, NodePtr r)
        : byte(0), freq(l->freq + r->freq),
          left(move(l)), right(move(r)) {}

    bool isLeaf() const {
        return !left && !right;
    }
};

struct NodeCmp {
    bool operator()(const NodePtr& a, const NodePtr& b) const {
        return a->freq > b->freq;
    }
};

NodePtr buildTree(const array<uint64_t, 256>& freqs) {

    priority_queue<NodePtr, vector<NodePtr>, NodeCmp> pq;

    for (int i = 0; i < 256; ++i) {
        if (freqs[i] > 0) {
            pq.push(make_shared<Node>((uint8_t)i, freqs[i]));
        }
    }

    if (pq.empty())
        return nullptr;

    while (pq.size() > 1) {

        NodePtr a = pq.top();
        pq.pop();

        NodePtr b = pq.top();
        pq.pop();

        pq.push(make_shared<Node>(a, b));
    }

    return pq.top();
}

void buildCodes(const NodePtr& root, const string& code,
                array<string, 256>& codes) {

    if (!root)
        return;

    if (root->isLeaf()) {
        codes[root->byte] = code.empty() ? "1" : code;
        return;
    }

    buildCodes(root->left, code + "0", codes);
    buildCodes(root->right, code + "1", codes);
}

class BitWriter {
    ostream& out;
    uint8_t buffer = 0;
    int bits = 0;

public:
    explicit BitWriter(ostream& o) : out(o) {}

    void writeBit(int b) {

        buffer = (uint8_t)((buffer << 1) | (b & 1));

        if (++bits == 8) {
            out.put((char)buffer);

            buffer = 0;
            bits = 0;
        }
    }

    void writeBits(const string& s) {

        for (char c : s)
            writeBit(c - '0');
    }

    int flush() {

        int pad = 0;

        if (bits > 0) {

            pad = 8 - bits;

            buffer = (uint8_t)(buffer << pad);

            out.put((char)buffer);

            buffer = 0;
            bits = 0;
        }

        return pad;
    }
};

class BitReader {
    istream& in;
    uint8_t buffer = 0;
    int bits = 0;

public:
    explicit BitReader(istream& i) : in(i) {}

    int readBit() {

        if (bits == 0) {

            int c = in.get();

            if (c == EOF)
                return -1;

            buffer = (uint8_t)c;
            bits = 8;
        }

        return (buffer >> (--bits)) & 1;
    }
};

static constexpr char MAGIC[4] = {'H', 'U', 'F', 'F'};

bool compress(const string& inPath, const string& outPath) {

    ifstream in(inPath, ios::binary);

    if (!in) {
        cerr << "Error: cannot open input file '" << inPath << "'\n";
        return false;
    }

    array<uint64_t, 256> freqs{};
    uint64_t totalBytes = 0;
    uint16_t distinct = 0;

    char c;

    while (in.get(c)) {
        freqs[(uint8_t)c]++;
        totalBytes++;
    }

    for (int i = 0; i < 256; ++i) {
        if (freqs[i] > 0)
            ++distinct;
    }

    ofstream out(outPath, ios::binary);

    if (!out) {
        cerr << "Error: cannot open output file '" << outPath << "'\n";
        return false;
    }

    out.write(MAGIC, 4);
    out.write((const char*)&totalBytes, sizeof(totalBytes));

    uint8_t padBits = 0;
    uint8_t distinctByte =
        (uint8_t)(distinct == 256 ? 0 : distinct);

    out.put((char)padBits);
    out.put((char)distinctByte);

    out.write((const char*)freqs.data(), sizeof(freqs));

    if (totalBytes == 0) {
        cout << "Compressed empty file.\n";
        return true;
    }

    NodePtr root = buildTree(freqs);

    array<string, 256> codes;

    buildCodes(root, "", codes);

    in.clear();
    in.seekg(0);

    BitWriter bw(out);

    while (in.get(c))
        bw.writeBits(codes[(uint8_t)c]);

    padBits = (uint8_t)bw.flush();

    out.seekp(4 + 8);
    out.put((char)padBits);
    out.seekp(0, ios::end);

    streamoff outSize = out.tellp();

    double ratio = (totalBytes == 0) ? 0.0
        : (1.0 - (double)outSize / (double)totalBytes) * 100.0;

    cout << fixed << setprecision(2);

    cout << "--------------------------------------------------\n";
    cout << " Compression Statistics\n";
    cout << "--------------------------------------------------\n";

    cout << "  Input file        : " << inPath << '\n';
    cout << "  Output file       : " << outPath << '\n';
    cout << "  Original size     : " << totalBytes << " bytes\n";
    cout << "  Compressed size   : " << outSize << " bytes\n";

    cout << "  Space saved       : "
         << (static_cast<int64_t>(totalBytes) -
             static_cast<int64_t>(outSize))
         << " bytes\n";

    cout << "  Reduction         : " << ratio << " %\n";
    cout << "  Distinct symbols  : " << distinct << " / 256\n";
    cout << "  Pad bits          : " << (int)padBits << '\n';

    cout << "--------------------------------------------------\n";

    return true;
}

bool decompress(const string& inPath, const string& outPath) {

    ifstream in(inPath, ios::binary);

    if (!in) {
        cerr << "Error: cannot open input file '" << inPath << "'\n";
        return false;
    }

    char magic[4];

    in.read(magic, 4);

    if (in.gcount() != 4 || memcmp(magic, MAGIC, 4) != 0) {
        cerr << "Error: corrupt file — bad magic header\n";
        return false;
    }

    uint64_t totalBytes = 0;

    in.read((char*)&totalBytes, sizeof(totalBytes));

    if (in.gcount() != (streamsize)sizeof(totalBytes)) {
        cerr << "Error: corrupt file — truncated header\n";
        return false;
    }

    uint8_t padBits = 0;

    if (totalBytes > 0) {

        in.read((char*)&padBits, 1);

        if (in.gcount() != 1) {
            cerr << "Error: corrupt file — missing pad-bits field\n";
            return false;
        }
    }
    else {
        in.seekg(1, ios::cur);
    }

    uint8_t distinctByte = 0;

    in.read((char*)&distinctByte, 1);

    uint16_t distinct;

    if (totalBytes == 0)
        distinct = 0;
    else
        distinct = (distinctByte == 0) ? 256 : distinctByte;

    array<uint64_t, 256> freqs{};

    in.read((char*)freqs.data(), sizeof(freqs));

    if (in.gcount() != (streamsize)sizeof(freqs)) {
        cerr << "Error: corrupt file — frequency table truncated\n";
        return false;
    }

    uint64_t check = 0;
    uint16_t seen = 0;

    for (int i = 0; i < 256; ++i) {

        check += freqs[i];

        if (freqs[i] > 0)
            ++seen;
    }

    if (check != totalBytes) {

        cerr << "Error: corrupt file — frequency total mismatch "
             << "(declared " << totalBytes
             << ", actual " << check << ")\n";

        return false;
    }

    if (seen != distinct) {
        cerr << "Error: corrupt file — distinct-symbol count mismatch\n";
        return false;
    }

    ofstream out(outPath, ios::binary);

    if (!out) {
        cerr << "Error: cannot open output file '" << outPath << "'\n";
        return false;
    }

    if (totalBytes == 0) {
        cout << "Decompressed empty file (0 bytes).\n";
        return true;
    }

    NodePtr root = buildTree(freqs);

    if (!root) {
        cerr << "Error: corrupt file — cannot rebuild tree\n";
        return false;
    }

    BitReader br(in);

    uint64_t written = 0;

    if (root->isLeaf()) {

        for (uint64_t i = 0; i < totalBytes; ++i)
            out.put((char)root->byte);

        written = totalBytes;
    }
    else {

        while (written < totalBytes) {

            NodePtr cur = root;

            while (!cur->isLeaf()) {

                int bit = br.readBit();

                if (bit < 0) {

                    cerr << "Error: corrupt file — unexpected EOF in stream "
                         << "(decoded " << written
                         << " of " << totalBytes << " bytes)\n";

                    return false;
                }

                cur = bit ? cur->right : cur->left;

                if (!cur) {
                    cerr << "Error: corrupt file — invalid Huffman tree\n";
                    return false;
                }
            }

            out.put((char)cur->byte);
            ++written;
        }
    }

    in.clear();
    in.seekg(0, ios::end);

    streamoff compSize = in.tellg();

    cout << "--------------------------------------------------\n";
    cout << " Decompression Statistics\n";
    cout << "--------------------------------------------------\n";

    cout << "  Input file        : " << inPath << '\n';
    cout << "  Output file       : " << outPath << '\n';
    cout << "  Compressed size   : " << compSize << " bytes\n";
    cout << "  Decompressed size : " << written << " bytes\n";
    cout << "  Distinct symbols  : " << seen << " / 256\n";

    cout << "--------------------------------------------------\n";

    return true;
}

void usage(const char* prog) {

    cerr <<
        "==============================================================\n"
        " Huffman File Compression Utility\n"
        "==============================================================\n"
        " Usage:\n"
        "   " << prog
        << " compress   <input> <output>   Compress a file\n"
        "   " << prog
        << " decompress <input> <output>   Decompress a file\n"
        "==============================================================\n";
}

int main(int argc, char* argv[]) {

    if (argc != 4) {
        usage(argv[0]);
        return 1;
    }

    string mode = argv[1];
    string in = argv[2];
    string out = argv[3];

    if (mode == "compress") {
        return compress(in, out) ? 0 : 1;
    }

    if (mode == "decompress") {
        return decompress(in, out) ? 0 : 1;
    }

    usage(argv[0]);

    return 1;
}