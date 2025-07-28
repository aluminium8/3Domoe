#include <iostream>
#include <future>
#include "CommandProcessor.hpp"
#include "GenerateCentroidsCartridge.hpp"

int main() {
    CommandProcessor processor;
    
    // 1. カートリッジのインスタンスを作成して登録
    processor.register_cartridge("generateCentroids", GenerateCentroidsCartridge{});
    // 他のカートリッジも同様に登録...
    // processor.register_cartridge("createMesh", CreateMeshCartridge{});

    // 2. GUIからコマンド発行をシミュレート
    const std::string command_name = "generateCentroids";
    const std::string input_toml = R"(input_mesh_id = 101)";

    // 3. タスクを作成し、ワーカースレッドへ渡す
    auto [task, future] = processor.create_task(command_name, input_toml);
    std::cout << "\nClient: Dispatching '" << command_name << "' task..." << std::endl;
// 修正後
auto handle = std::async(std::launch::async, std::move(task));

    // 4. 結果を待って表示
    std::cout << "Client: Waiting for result..." << std::endl;
    CommandResult result = future.get();
    
    // 5. 結果を処理
    if (auto* success = std::get_if<SuccessResult>(&result)) {
        std::cout << "Client: Task succeeded!" << std::endl;
        std::cout << "  Raw Output TOML: " << success->output_toml << std::endl;

        // 必要に応じて、クライアント側でOutputオブジェクトにデシリアライズ
        auto output = rfl::toml::read<GenerateCentroidsCartridge::Output>(success->output_toml);
        if(output) {
            std::cout << "  Deserialized Message: " << output->message << std::endl;
            std::cout << "  New Point Cloud ID: " << output->generated_point_cloud_id << std::endl;
        }

    } else if (auto* error = std::get_if<ErrorResult>(&result)) {
        std::cout << "Client: Task failed! Reason: " << error->error_message << std::endl;
    }
    
    return 0;
}