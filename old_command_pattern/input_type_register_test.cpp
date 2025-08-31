// main.cpp

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <memory>
#include <sstream>

#include <Eigen/Dense>

// --- 必要なライブラリのインクルード ---
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <rfl/fields.hpp>
#include <rfl/to_view.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>


// =========================================================================
// 1. フレームワークの基本定義 (通常は別ヘッダに記述)
// =========================================================================

// UIでサポートするフィールドの型
using SupportedFieldType = std::variant<
    std::monostate, // 未設定またはサポート外
    int, unsigned int, size_t,
    float, double,
    bool,
    std::string>;

// フィールドのメタ情報
struct FieldInfo
{
    std::string name;
    SupportedFieldType default_value;
};

// ラムダのオーバーロードセットを作成するヘルパー
template <class... Ts>
struct overload : Ts...
{
    using Ts::operator()...;
};
template <class... Ts>
overload(Ts...) -> overload<Ts...>;


// =========================================================================
// 2. ユーザーが定義するコマンド（カートリッジ）
// =========================================================================

// --- コマンドA: オブジェクトを生成するコマンド ---
struct CreateObjectCartridge
{
    struct Input
    {
        std::string object_name = "MyObject";
        double size = 10.0;
        int type = 1;
        bool is_visible = true;
    };
    struct Output {}; // サンプルなのでOutputは空
};

// --- コマンドB: メッシュを処理するコマンド ---
struct ProcessMeshCartridge
{
    struct Input
    {
        unsigned int mesh_id = 1;
        float smoothing_factor = 0.5f;
        size_t iterations = 5;
    };
    struct Output {};
};


// =========================================================================
// 3. メタ情報管理クラス (CommandProcessorの簡易版)
// =========================================================================
class CommandMetaManager
{
public:
    template <typename Cartridge>
    void register_command(const std::string &name)
    {
        using InputType = typename Cartridge::Input;
        InputType default_input;
        const auto view = rfl::to_view(default_input);
        
        std::vector<FieldInfo> fields;
        rfl::for_each(view.fields(), [&](const auto &field) {
            using FieldType = std::remove_cvref_t<decltype(field.value())>;
            FieldInfo info;
            info.name = field.name();

            if constexpr (rfl::variant_alternative<FieldType, SupportedFieldType>) {
                info.default_value = field.value();
            } else {
                info.default_value = std::monostate{};
            }
            fields.push_back(info);
        });

        command_fields_[name] = fields;
        command_names_.push_back(name);
        std::cout << "Registered command: " << name << std::endl;
    }

    const std::vector<FieldInfo>& get_fields(const std::string &name) const
    {
        static const std::vector<FieldInfo> empty;
        auto it = command_fields_.find(name);
        return (it != command_fields_.end()) ? it->second : empty;
    }

    const std::vector<std::string>& get_command_names() const { return command_names_; }

private:
    std::vector<std::string> command_names_;
    std::map<std::string, std::vector<FieldInfo>> command_fields_;
};


// =========================================================================
// 4. ImGui UIジェネレータ
// =========================================================================
class ImGuiUIGenerator
{
public:
    ImGuiUIGenerator(const std::vector<FieldInfo> &fields)
    {
        for (const auto &field : fields)
        {
            if (std::holds_alternative<std::string>(field.default_value)) {
                string_buffers_[field.name] = std::get<std::string>(field.default_value);
            } else if (!std::holds_alternative<std::monostate>(field.default_value)) {
                value_buffers_[field.name] = field.default_value;
            }
        }
    }

    void render_ui()
    {
        for (auto &[name, value] : string_buffers_) {
            char buffer[256];
            strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
            buffer[sizeof(buffer) - 1] = 0;
            if (ImGui::InputText(name.c_str(), buffer, sizeof(buffer))) value = buffer;
        }

        for (auto &[name, value] : value_buffers_) {
            std::visit(overload{
                [&](int &val)           { ImGui::InputInt(name.c_str(), &val); },
                [&](unsigned int &val)  { int temp = val; if(ImGui::InputInt(name.c_str(), &temp) && temp >=0) val = temp; },
                [&](size_t &val)        { int temp = val; if(ImGui::InputInt(name.c_str(), &temp) && temp >=0) val = temp; },
                [&](float &val)         { ImGui::InputFloat(name.c_str(), &val); },
                [&](double &val)        { ImGui::InputDouble(name.c_str(), &val); },
                [&](bool &val)          { ImGui::Checkbox(name.c_str(), &val); },
                [&](const auto &)       { ImGui::Text("%s: (Read-only)", name.c_str()); }
            }, value);
        }
    }

    std::string get_json_string() const
    {
        std::stringstream ss;
        ss << "{";
        bool first = true;
        auto add_kv = [&](const std::string &key, const auto &val) {
            if (!first) ss << ",";
            ss << "\"" << key << "\":";
             if constexpr (std::is_same_v<std::decay_t<decltype(val)>, bool>) ss << (val ? "true" : "false");
             else if constexpr (std::is_convertible_v<std::decay_t<decltype(val)>, std::string>) ss << "\"" << val << "\"";
             else ss << val;
            first = false;
        };
        for (const auto &[name, val] : string_buffers_) add_kv(name, val);
        for (const auto &[name, val] : value_buffers_) {
            std::visit([&](auto &&v) {
                if constexpr (!std::is_same_v<std::decay_t<decltype(v)>, std::monostate>) add_kv(name, v);
            }, val);
        }
        ss << "}";
        return ss.str();
    }

private:
    std::map<std::string, std::string> string_buffers_;
    std::map<std::string, SupportedFieldType> value_buffers_;
};


// =========================================================================
// 5. メインアプリケーション
// =========================================================================
int main()
{
    // --- GLFW/GLAD/ImGuiの初期化 ---
    glfwInit();
    const char* glsl_version = "#version 130";
    GLFWwindow* window = glfwCreateWindow(800, 600, "Dynamic UI Generation Sample", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // VSync有効
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImGui::StyleColorsDark();

    // --- アプリケーションのセットアップ ---
    CommandMetaManager manager;
    manager.register_command<CreateObjectCartridge>("Create Object");
    manager.register_command<ProcessMeshCartridge>("Process Mesh");

    const auto& command_names = manager.get_command_names();
    int selected_command_idx = 0;
    std::unique_ptr<ImGuiUIGenerator> ui_generator =
        std::make_unique<ImGuiUIGenerator>(manager.get_fields(command_names[0]));
    
    std::string last_generated_json;

    // --- メインループ ---
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --- UIの描画 ---
        ImGui::Begin("Command Controller");

        if (ImGui::BeginCombo("Command", command_names[selected_command_idx].c_str()))
        {
            for (int i = 0; i < command_names.size(); ++i)
            {
                const bool is_selected = (selected_command_idx == i);
                if (ImGui::Selectable(command_names[i].c_str(), is_selected))
                {
                    selected_command_idx = i;
                    // コンボボックスで選択が変わったら、UIジェネレータを再作成
                    ui_generator = std::make_unique<ImGuiUIGenerator>(manager.get_fields(command_names[i]));
                }
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();
        
        if (ui_generator)
        {
            ui_generator->render_ui();
        }

        if (ImGui::Button("Execute"))
        {
            if(ui_generator) {
                last_generated_json = ui_generator->get_json_string();
                std::cout << "Generated JSON: " << last_generated_json << std::endl;
            }
        }

        if(!last_generated_json.empty()){
            ImGui::Separator();
            ImGui::Text("Last Generated JSON:");
            ImGui::TextWrapped("%s", last_generated_json.c_str());
        }

        ImGui::End();

        // --- レンダリング ---
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // --- 後片付け ---
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}