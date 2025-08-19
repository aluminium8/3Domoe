#include <iostream>
#include <vector>
#include <future>

#include <Eigen/Dense>


#include "_CommandProcessor.hpp"
#include "_GenerateCentroidsCartridge.hpp"

#include <rfl/json.hpp>



int main()
{

    struct MYmatrix
    {
        std::vector<std::vector<double >>mat;
        int ID;
    };
    const auto mat1 =
        MYmatrix{.mat = {{2,2},{3,3}},
               .ID = 45};
    // We can now write into and read from a JSON string.
    const std::string json_string = rfl::json::write(mat1);
    std::cout<<json_string<<std::endl;


    struct eigenmatrix
    {
        Eigen::MatrixXd mat;
        int ID;
    };
    const auto mat2 =
        eigenmatrix{.mat =Eigen::MatrixXd::Random(3, 4),
               .ID = 45};


    //// We can now write into and read from a JSON string.
    //const std::string json2_string = rfl::json::write(mat2);
    //std::cout<<json2_string<<std::endl;

    CommandProcessor processor;

    // 1. カートリッジのインスタンスを作成して登録
    processor.register_cartridge("generateCentroids", GenerateCentroidsCartridge{});
    // 他のカートリッジも同様に登録...
    // processor.register_cartridge("createMesh", CreateMeshCartridge{});

    // 2. GUIからコマンド発行をシミュレート
    const std::string command_name = "generateCentroids";
    const std::string input_json = R"(input_mesh_id = 101)";

    // 3. タスクを作成し、ワーカースレッドへ渡す
    auto [task, future] = processor.create_task(command_name, input_json);
    std::cout << "\nClient: Dispatching '" << command_name << "' task..." << std::endl;
    // 修正後
    auto handle = std::async(std::launch::async, std::move(task));

    // 4. 結果を待って表示
    std::cout << "Client: Waiting for result..." << std::endl;
    CommandResult result = future.get();

    // 5. 結果を処理
    if (auto *success = std::get_if<SuccessResult>(&result))
    {
        std::cout << "Client: Task succeeded!" << std::endl;
        std::cout << "  Raw Output json: " << success->output_json << std::endl;

        // 必要に応じて、クライアント側でOutputオブジェクトにデシリアライズ
        auto output = rfl::json::read<GenerateCentroidsCartridge::Output>(success->output_json);
        if (output)
        {
            std::cout << "  Deserialized Message: " << output->message << std::endl;
            std::cout << "  New Point Cloud ID: " << output->generated_point_cloud_id << std::endl;
        }
    }
    else if (auto *error = std::get_if<ErrorResult>(&result))
    {
        std::cout << "Client: Task failed! Reason: " << error->error_message << std::endl;
    }

    return 0;
}