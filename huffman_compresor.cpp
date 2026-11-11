#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <memory>
#include <cstdint>
#include <string>
#include <cmath>
#include <iomanip>
#include <chrono>

struct HuffmanNode {
    uint8_t character;
    uint64_t frequency;
    std::shared_ptr<HuffmanNode> left;
    std::shared_ptr<HuffmanNode> right;

    HuffmanNode(uint8_t ch, uint64_t freq)
        : character(ch), frequency(freq), left(nullptr), right(nullptr) {}

    HuffmanNode(std::shared_ptr<HuffmanNode> l, std::shared_ptr<HuffmanNode> r)
        : character(0), frequency(l->frequency + (r ? r->frequency : 0)), left(l), right(r) {}

    bool is_leaf() const {
        return !left && !right;
    }
};

struct NodeComparator {
    bool operator()(const std::shared_ptr<HuffmanNode>& a,
                    const std::shared_ptr<HuffmanNode>& b) const {
        if (a->frequency != b->frequency)
            return a->frequency > b->frequency; // Monticulo de minimos
        return a->character > b->character;
    }
};

std::shared_ptr<HuffmanNode> build_huffman_tree(const std::vector<uint64_t>& frequencies) {
    std::priority_queue<std::shared_ptr<HuffmanNode>,
                        std::vector<std::shared_ptr<HuffmanNode>>,
                        NodeComparator> min_heap;

    for (int i = 0; i < 256; ++i) {
        if (frequencies[i] > 0) {
            min_heap.push(std::make_shared<HuffmanNode>(static_cast<uint8_t>(i), frequencies[i]));
        }
    }

    if (min_heap.empty()) {
        return nullptr;
    }

    if (min_heap.size() == 1) {
        auto single = min_heap.top();
        min_heap.pop();
        return std::make_shared<HuffmanNode>(single, nullptr);
    }

    while (min_heap.size() > 1) {
        auto left = min_heap.top(); min_heap.pop();
        auto right = min_heap.top(); min_heap.pop();
        auto parent = std::make_shared<HuffmanNode>(left, right);
        min_heap.push(parent);
    }

    return min_heap.top();
}


class BitWriter {
private:
    std::ostream& out;
    uint8_t buffer;
    int bit_count;
    uint64_t bytes_written;

public:
    explicit BitWriter(std::ostream& os) : out(os), buffer(0), bit_count(0), bytes_written(0) {}

    void write_bit(int bit) {
        buffer = static_cast<uint8_t>((buffer << 1) | (bit & 1));
        bit_count++;
        if (bit_count == 8) {
            out.put(static_cast<char>(buffer));
            bytes_written++;
            buffer = 0;
            bit_count = 0;
        }
    }

    void write_byte(uint8_t byte) {
        for (int i = 7; i >= 0; --i) {
            write_bit((byte >> i) & 1);
        }
    }

    void write_code(const std::string& code) {
        for (char c : code) {
            write_bit(c == '1' ? 1 : 0);
        }
    }

    void flush() {
        if (bit_count > 0) {
            buffer <<= (8 - bit_count);
            out.put(static_cast<char>(buffer));
            bytes_written++;
            buffer = 0;
            bit_count = 0;
        }
    }

    uint64_t get_bytes_written() const { return bytes_written; }
};

class BitReader {
private:
    std::istream& in;
    uint8_t buffer;
    int bits_remaining;

public:
    explicit BitReader(std::istream& is) : in(is), buffer(0), bits_remaining(0) {}

    bool read_bit(int& bit) {
        if (bits_remaining == 0) {
            int c = in.get();
            if (c == EOF) return false;
            buffer = static_cast<uint8_t>(c);
            bits_remaining = 8;
        }
        bit = (buffer >> (bits_remaining - 1)) & 1;
        bits_remaining--;
        return true;
    }

    bool read_byte(uint8_t& byte) {
        byte = 0;
        for (int i = 0; i < 8; ++i) {
            int b = 0;
            if (!read_bit(b)) return false;
            byte = static_cast<uint8_t>((byte << 1) | b);
        }
        return true;
    }
};


void generate_codes(const std::shared_ptr<HuffmanNode>& node,
                    const std::string& prefix,
                    std::vector<std::string>& codes) {
    if (!node) return;
    if (node->is_leaf()) {
        codes[node->character] = prefix.empty() ? "0" : prefix;
        return;
    }
    if (node->left) generate_codes(node->left, prefix + "0", codes);
    if (node->right) generate_codes(node->right, prefix + "1", codes);
}

void serialize_tree(const std::shared_ptr<HuffmanNode>& node, BitWriter& writer) {
    if (!node) return;
    if (node->is_leaf()) {
        writer.write_bit(1);
        writer.write_byte(node->character);
        return;
    }
    writer.write_bit(0);
    serialize_tree(node->left, writer);
    if (node->right) {
        serialize_tree(node->right, writer);
    }
}

std::shared_ptr<HuffmanNode> deserialize_tree(BitReader& reader, bool is_single_symbol) {
    int bit = 0;
    if (!reader.read_bit(bit)) return nullptr;
    if (bit == 1) {
        uint8_t ch = 0;
        reader.read_byte(ch);
        return std::make_shared<HuffmanNode>(ch, 0);
    }
    auto left = deserialize_tree(reader, false);
    if (is_single_symbol) {
        return std::make_shared<HuffmanNode>(left, nullptr);
    }
    auto right = deserialize_tree(reader, false);
    return std::make_shared<HuffmanNode>(left, right);
}


bool compress_file(const std::string& input_path, const std::string& output_path) {
    std::ifstream in(input_path, std::ios::binary);
    if (!in) return false;

    std::vector<uint64_t> freq(256, 0);
    uint64_t total_chars = 0;
    char ch;
    while (in.get(ch)) {
        freq[static_cast<uint8_t>(ch)]++;
        total_chars++;
    }

    uint16_t unique_symbols = 0;
    for (int i = 0; i < 256; ++i) {
        if (freq[i] > 0) unique_symbols++;
    }

    auto root = build_huffman_tree(freq);
    std::vector<std::string> codes(256);
    generate_codes(root, "", codes);

    std::ofstream out(output_path, std::ios::binary);
    if (!out) return false;

    out.write(reinterpret_cast<const char*>(&total_chars), sizeof(total_chars));
    out.write(reinterpret_cast<const char*>(&unique_symbols), sizeof(unique_symbols));

    if (total_chars == 0) return true;

    BitWriter writer(out);
    serialize_tree(root, writer);

    in.clear();
    in.seekg(0, std::ios::beg);
    while (in.get(ch)) {
        writer.write_code(codes[static_cast<uint8_t>(ch)]);
    }
    writer.flush();
    return true;
}

bool decompress_file(const std::string& input_path, const std::string& output_path) {
    std::ifstream in(input_path, std::ios::binary);
    if (!in) return false;

    uint64_t total_chars = 0;
    uint16_t unique_symbols = 0;
    in.read(reinterpret_cast<char*>(&total_chars), sizeof(total_chars));
    in.read(reinterpret_cast<char*>(&unique_symbols), sizeof(unique_symbols));

    std::ofstream out(output_path, std::ios::binary);
    if (!out) return false;
    if (total_chars == 0) return true;

    BitReader reader(in);
    bool is_single = (unique_symbols == 1);
    auto root = deserialize_tree(reader, is_single);

    for (uint64_t i = 0; i < total_chars; ++i) {
        auto curr = root;
        while (!curr->is_leaf()) {
            int bit = 0;
            if (!reader.read_bit(bit)) break;
            curr = (bit == 0) ? curr->left : curr->right;
        }
        out.put(static_cast<char>(curr->character));
    }
    return true;
}


void analizar_mensaje_fase1() {
    std::string M = "ABRACADABRA_ALGORITMO_AVANZADO_ESCOM";
    std::vector<uint64_t> freq(256, 0);
    for (unsigned char c : M) freq[c]++;

    double N = static_cast<double>(M.size());
    double entropia = 0.0;
    for (int i = 0; i < 256; ++i) {
        if (freq[i] > 0) {
            double p = freq[i] / N;
            entropia -= p * std::log2(p);
        }
    }

    auto root = build_huffman_tree(freq);
    std::vector<std::string> codes(256);
    generate_codes(root, "", codes);

    double L_prom = 0.0;
    for (int i = 0; i < 256; ++i) {
        if (freq[i] > 0) {
            double p = freq[i] / N;
            L_prom += p * codes[i].size();
        }
    }

    std::cout << "=== FASE 1: ANALISIS DEL MENSAJE ===\n";
    std::cout << "Mensaje: " << M << " (N = " << M.size() << ")\n";
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "Entropia de Shannon H(X): " << entropia << " bits/simbolo\n";
    std::cout << "Longitud promedio L:      " << L_prom << " bits/simbolo\n";
    std::cout << "Desigualdad de Shannon:   " << entropia << " <= " << L_prom
              << " < " << (entropia + 1.0) << "\n\n";
}

int main(int argc, char* argv[]) {
    if (argc == 4) {
        std::string mode = argv[1];
        if (mode == "-c") {
            compress_file(argv[2], argv[3]);
            std::cout << "Archivo comprimido en: " << argv[3] << "\n";
        } else if (mode == "-d") {
            decompress_file(argv[2], argv[3]);
            std::cout << "Archivo descomprimido en: " << argv[3] << "\n";
        }
        return 0;
    }

    analizar_mensaje_fase1();

    std::cout << "=== PRUEBA CASO BORDE: 10^7 CARACTERES 'A' ===\n";
    const std::string f_in = "test_homogeneo.txt";
    const std::string f_comp = "test_homogeneo.huff";
    const std::string f_out = "test_homogeneo_rec.txt";

    {
        std::ofstream os(f_in, std::ios::binary);
        std::string bloque(10000, 'A');
        for (int i = 0; i < 1000; ++i) os.write(bloque.data(), bloque.size());
    }

    compress_file(f_in, f_comp);
    decompress_file(f_comp, f_out);

    std::ifstream orig(f_in, std::ios::binary | std::ios::ate);
    std::ifstream comp(f_comp, std::ios::binary | std::ios::ate);
    std::ifstream rec(f_out, std::ios::binary | std::ios::ate);

    double s_orig = orig.tellg();
    double s_comp = comp.tellg();
    double s_rec = rec.tellg();

    std::cout << "Tamano Original:     " << static_cast<uint64_t>(s_orig) << " bytes\n";
    std::cout << "Tamano Comprimido:   " << static_cast<uint64_t>(s_comp) << " bytes\n";
    std::cout << "Tamano Recuperado:   " << static_cast<uint64_t>(s_rec) << " bytes\n";
    std::cout << "Ratio de Compresion: " << (s_comp / s_orig) * 100.0 << "%\n";

    std::remove(f_in.c_str());
    std::remove(f_comp.c_str());
    std::remove(f_out.c_str());
    return 0;
}
