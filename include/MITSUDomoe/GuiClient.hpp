#pragma once

#include "BaseClient.hpp"

namespace MITSU_Domoe
{

class GuiClient : public BaseClient
{
public:
    GuiClient();
    ~GuiClient() = default;

    void run() override;
};

}
