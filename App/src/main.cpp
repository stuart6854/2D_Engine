#include "core/application.hpp"

#include <memory>

int main(int /*argc*/, char** /*argv*/)
{
    using namespace app;

    core::ApplicationInfo appInfo{};
    appInfo.name = "2D Engine";

    auto app = std::make_unique<core::Application>(appInfo);
    app->run();

    return 0;
}