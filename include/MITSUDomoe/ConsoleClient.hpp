#pragma once

#include "BaseClient.hpp"

namespace MITSU_Domoe
{

class ConsoleClient : public BaseClient
{
public:
    ConsoleClient();
    ~ConsoleClient() = default;

    void run() override;
};

}
