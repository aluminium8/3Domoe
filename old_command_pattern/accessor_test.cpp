#include <any>
#include <concepts>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <rfl.hpp>
#include <rfl/to_view.hpp>
#include <utility> // for std::index_sequence

// =================================================================
// Step 0: アプリケーションの基本コンセプト (普段は別ファイル)
// =================================================================

// 全てのカートリッジが満たすべき規約
template <typename T>
concept Cartridge = requires {
    typename T::Input;
    typename T::Output;
};

// =================================================================
// Step 1: 自動登録のためのレジストリ (普段は別ファイル)
// =================================================================
namespace MITSUDomoe
{

    // --- 結果ハンドラ用のレジストリ ---
    using ResultHandlerFunc = std::function<void(const std::any &)>;
    class ResultHandlerRegistry
    {
    public:
        static ResultHandlerRegistry &get_instance()
        {
            static ResultHandlerRegistry instance;
            return instance;
        }
        void register_handler(const std::string &name, ResultHandlerFunc func)
        {
            handlers_[name] = std::move(func);
        }
        void handle(const std::string &name, const std::any &result) const
        {
            if (auto it = handlers_.find(name); it != handlers_.end())
            {
                it->second(result);
            }
            else
            {
                std::cout << "  [Handler] Error: No handler found for '" << name << "'." << std::endl;
            }
        }

    private:
        ResultHandlerRegistry() = default;
        std::map<std::string, ResultHandlerFunc> handlers_;
    };

    // --- フィールド値アクセサ用のレジストリ ---
    using AccessorFunc = std::function<std::any(const std::any &)>;
    class ResultAccessorRegistry
    {
    public:
        static ResultAccessorRegistry &get_instance()
        {
            static ResultAccessorRegistry instance;
            return instance;
        }
        void register_accessor(const std::string &cmd, const std::string &field, AccessorFunc func)
        {
            accessors_[cmd][field] = std::move(func);
        }
        std::any get_field(const std::string &cmd, const std::string &field, const std::any &result) const
        {
            if (auto cmd_it = accessors_.find(cmd); cmd_it != accessors_.end())
            {
                if (auto field_it = cmd_it->second.find(field); field_it != cmd_it->second.end())
                {
                    return field_it->second(result);
                }
            }
            return {};
        }

    private:
        ResultAccessorRegistry() = default;
        std::map<std::string, std::map<std::string, AccessorFunc>> accessors_;
    };

    template <Cartridge C>
    class ResultHandlerRegistrar
    {
    public:
        ResultHandlerRegistrar(const std::string &name)
        {
            ResultHandlerRegistry::get_instance().register_handler(name,
                                                                   [name](const std::any &any_val) { // nameをキャプチャ
                                                                       if (const auto *output = std::any_cast<typename C::Output>(&any_val))
                                                                       {
                                                                           std::cout << "  [Handler] Handling result for '" << name << "':" << std::endl;

                                                                           // rfl::for_each の代わりに std::apply を使用
                                                                           std::apply(
                                                                               [&](auto &&...fields)
                                                                               {
                                                                                   ([&](const auto &field)
                                                                                    { std::cout << "    - " << field.name() << ": (value)" << std::endl; }(fields), ...);
                                                                               },
                                                                               rfl::fields<typename C::Output>());
                                                                       }
                                                                   });
        }
    };


    template <Cartridge C>
    class ResultAccessorRegistrar
    {
    public:
        ResultAccessorRegistrar(const std::string &name)
        {
            auto &registry = ResultAccessorRegistry::get_instance();
            const auto fields_tuple = rfl::fields<typename C::Output>();
            constexpr auto num_fields = std::tuple_size_v<std::remove_cvref_t<decltype(fields_tuple)>>;
            const auto indices = std::make_index_sequence<num_fields>();

            [&]<size_t... Is>(std::index_sequence<Is...>)
            {
                (
                    [&]
                    {
                        const auto &field = std::get<Is>(fields_tuple);
                        const std::string field_name = field.name().c_str();
                        registry.register_accessor(name, field_name,
                                                   [](const std::any &any_val) -> std::any
                                                   {
                                                       if (const auto *output = std::any_cast<typename C::Output>(&any_val))
                                                       {
                                                           const auto view = rfl::to_view(*output);

                                                           // ★★★★★★★★★★★★★★★★★★★★★★★★★★★
                                                           // 最終修正：get<Is>()はポインタを返すので、`*`でデリファレンスする
                                                           return *view.template get<Is>();
                                                           // ★★★★★★★★★★★★★★★★★★★★★★★★★★★
                                                       }
                                                       return {};
                                                   });
                    }(),
                    ...);
            }(indices);
        }
    };
} // namespace MITSUDomoe

// =================================================================
// Step 2: カートリッジの定義 (新しい処理を追加する際はここを真似る)
// =================================================================

// --- 1つ目のカートリッジ ---
class CreateCubeCartridge
{
public:
    struct Input
    {
    };
    struct Output
    {
        unsigned int mesh_id;
        std::string name;
        double size;
    };
};
static_assert(Cartridge<CreateCubeCartridge>);
// ★この2行だけで、このカートリッジの結果の扱い方がシステムに自動登録される
static MITSUDomoe::ResultHandlerRegistrar<CreateCubeCartridge> create_cube_handler_registrar("createCube");
static MITSUDomoe::ResultAccessorRegistrar<CreateCubeCartridge> create_cube_accessor_registrar("createCube");

// --- 2つ目のカートリッジ ---
class SmoothMeshCartridge
{
public:
    struct Input
    {
    };
    struct Output
    {
        unsigned int result_mesh_id;
        float smoothing_level;
        std::string message;
    };
};
static_assert(Cartridge<SmoothMeshCartridge>);
// ★同様に2行追加するだけ
static MITSUDomoe::ResultHandlerRegistrar<SmoothMeshCartridge> smooth_mesh_handler_registrar("smoothMesh");
static MITSUDomoe::ResultAccessorRegistrar<SmoothMeshCartridge> smooth_mesh_accessor_registrar("smoothMesh");

// =================================================================
// Step 3: メインロジック (実際のアプリケーション)
// =================================================================
int main()
{
    std::cout << "--- Automatic Registration Sample ---" << std::endl;
    // mainが始まる前に、全てのstaticなRegistrarが動き、登録が完了している

    // -----------------------------------------------------------------
    std::cout << "\nScenario 1: Generic 'handle' of a result" << std::endl;
    // -----------------------------------------------------------------
    {
        // (1) "createCube"処理が実行され、結果がanyで返されたと仮定
        CreateCubeCartridge::Output cube_result = {101, "MyCube", 5.0};
        std::any result_any = cube_result;
        std::string command_name = "createCube";

        // (2) コマンド名をキーに、対応するハンドラを呼び出す
        auto &handler_registry = MITSUDomoe::ResultHandlerRegistry::get_instance();
        handler_registry.handle(command_name, result_any);

        // (3) 存在しないコマンドのハンドラを呼んでみる
        handler_registry.handle("nonExistentCommand", result_any);
    }

    // -----------------------------------------------------------------
    std::cout << "\nScenario 2: Generic 'access' of a specific field" << std::endl;
    // -----------------------------------------------------------------
    {
        // (1) "smoothMesh"処理が実行され、結果がanyで返されたと仮定
        SmoothMeshCartridge::Output smooth_result = {202, 0.75f, "Smoothing complete"};
        std::any result_any = smooth_result;
        std::string command_name = "smoothMesh";

        // (2) コマンド名とフィールド名をキーに、特定の値を取り出す
        auto &accessor_registry = MITSUDomoe::ResultAccessorRegistry::get_instance();
        std::string field_to_get = "result_mesh_id";

        std::any field_value_any = accessor_registry.get_field(command_name, field_to_get, result_any);

        // (3) 取り出した値を、使いたい型にキャストして利用する
        if (const auto *id_ptr = std::any_cast<unsigned int>(&field_value_any))
        {
            std::cout << "  [Accessor] Successfully extracted '" << field_to_get << "'. Value: " << *id_ptr << std::endl;
            // このIDを次の処理に渡すことができる
        }
        else
        {
            std::cout << "  [Accessor] Failed to extract '" << field_to_get << "' or type was wrong." << std::endl;
        }

        // (4) 別のフィールド('message')も取り出してみる
        field_to_get = "message";
        field_value_any = accessor_registry.get_field(command_name, field_to_get, result_any);
        if (const auto *msg_ptr = std::any_cast<std::string>(&field_value_any))
        {
            std::cout << "  [Accessor] Successfully extracted '" << field_to_get << "'. Value: \"" << *msg_ptr << "\"" << std::endl;
        }
    }

    return 0;
}