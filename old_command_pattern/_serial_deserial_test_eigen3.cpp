#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>

#include "rfl.hpp"

// --- 型定義 (変更なし) ---
struct MESH_ID { int id; };
struct MESH { std::string data; };
struct TEXTURE_ID { int id; };
struct TEXTURE { std::string path; };

// 変換元 (Source)
struct SourceAsset {
    MESH_ID mesh_id;
    TEXTURE_ID texture_id;
    std::string name;
};

// 変換先 1 (Destination 1)
struct LoadedAsset {
    MESH mesh;
    TEXTURE texture;
    std::string name;
};

// 変換先 2 (Destination 2) - 一部のフィールドのみを持つ
struct AssetSummary {
    MESH mesh;
    std::string name;
};

// --- データストアとConverterクラス (前回と同じ) ---
std::unordered_map<int, MESH> mesh_store = {{101, {"mesh_data_for_101"}}};
std::unordered_map<int, TEXTURE> texture_store = {{5, {"/textures/tex_5.png"}}};

class AssetConverter {
private:
    const std::unordered_map<int, MESH>& mesh_store_ref_;
    const std::unordered_map<int, TEXTURE>& texture_store_ref_;
    template <rfl::internal::StringLiteral str>
    static consteval auto remove_id_suffix() { /* ... 実装は省略 ... */ 
        constexpr auto sv = str.string_view();
        constexpr auto new_sv = sv.substr(0, sv.size() - 3);
        constexpr auto new_size = new_sv.size();
        std::array<char, new_size + 1> arr{};
        for (size_t i = 0; i < new_size; ++i) arr[i] = new_sv[i];
        arr[new_size] = '\0';
        return rfl::internal::StringLiteral<new_size + 1>(arr);
    }
public:
    AssetConverter(const std::unordered_map<int, MESH>& ms, const std::unordered_map<int, TEXTURE>& ts)
        : mesh_store_ref_(ms), texture_store_ref_(ts) {}
    MESH operator()(const MESH_ID& id) const { return mesh_store_ref_.at(id.id); }
    TEXTURE operator()(const TEXTURE_ID& id) const { return texture_store_ref_.at(id.id); }

    template <typename To, typename From>
    To convert(const From& from) const {
        const auto from_nt = rfl::to_named_tuple(from);
        const auto to_nt = from_nt.transform([&](auto&& field) {
            using FieldType = std::remove_cvref_t<decltype(field)>;
            constexpr auto name_literal = FieldType::name_;
            if constexpr (name_literal.string_view().ends_with("_id")) {
                constexpr auto new_name_literal = remove_id_suffix<name_literal>();
                auto new_value = (*this)(field.get());
                return rfl::make_field<new_name_literal>(new_value);
            } else {
                return std::forward<decltype(field)>(field);
            }
        });
        return rfl::from_named_tuple<To>(to_nt);
    }
};

int main() {
    // データストアと変換元のオブジェクトを準備
    std::unordered_map<int, MESH> local_mesh_store = {{101, {"local_mesh_data"}}};
    std::unordered_map<int, TEXTURE> local_texture_store = {{5, {"/local/texture.png"}}};
    SourceAsset source = {{101}, {5}, "MyObject"};
    AssetConverter converter(local_mesh_store, local_texture_store);

    // --- 同じconvert関数を、異なる変換先を指定して呼び出す ---

    // 1. LoadedAssetに変換
    LoadedAsset loaded = converter.convert<LoadedAsset>(source);
    std::cout << "--- Converted to LoadedAsset ---" << std::endl;
    std::cout << "Name: " << loaded.name << std::endl;
    std::cout << "Mesh Data: " << loaded.mesh.data << std::endl;
    std::cout << "Texture Path: " << loaded.texture.path << std::endl;

    std::cout << std::endl;

    // 2. AssetSummaryに変換
    AssetSummary summary = converter.convert<AssetSummary>(source);
    std::cout << "--- Converted to AssetSummary ---" << std::endl;
    std::cout << "Name: " << summary.name << std::endl;
    std::cout << "Mesh Data: " << summary.mesh.data << std::endl;
    // (summaryにはtextureメンバがないのでアクセスできない)

    return 0;
}