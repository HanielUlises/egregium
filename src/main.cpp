#include "app.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {
void printHelp(const char* argv0) {
    std::printf(
        "usage: %s [options]\n"
        "  --shot \"<demo>;<output.png>[;yaw[;pitch[;distance[;colormap]]]]\"\n"
        "                          batch-capture a demo screenshot; repeatable.\n"
        "                          All --shot entries run in ONE window/process,\n"
        "                          in order, then the app exits. colormap is\n"
        "                          'diverging', 'heatmap', or omitted for the\n"
        "                          demo's own default. Example:\n"
        "                            --shot \"Klein Bottle;klein.png;20;15;9\"\n"
        "  --frames-per-shot <n>   frames rendered before each shot's capture (default 5)\n"
        "  --width <n> / --height <n>   window size (default 1280x800)\n"
        "  --help                  show this message\n",
        argv0);
}

App::Shot parseShot(const std::string& spec) {
    App::Shot shot;
    std::vector<std::string> fields;
    std::stringstream ss(spec);
    std::string field;
    while (std::getline(ss, field, ';')) fields.push_back(field);
    if (fields.size() < 2) throw std::runtime_error("--shot needs at least '<demo>;<output.png>': " + spec);
    shot.demoName = fields[0];
    shot.outputPath = fields[1];
    if (fields.size() > 2 && !fields[2].empty()) shot.yawDeg = static_cast<float>(std::atof(fields[2].c_str()));
    if (fields.size() > 3 && !fields[3].empty()) shot.pitchDeg = static_cast<float>(std::atof(fields[3].c_str()));
    if (fields.size() > 4 && !fields[4].empty()) shot.distance = static_cast<float>(std::atof(fields[4].c_str()));
    if (fields.size() > 5) shot.colormap = fields[5];
    return shot;
}
} // namespace

int main(int argc, char** argv) {
    App::Options opts;
    for (int i = 1; i < argc; ++i) {
        auto arg = [&](const char* name) { return std::strcmp(argv[i], name) == 0; };
        auto next = [&]() -> std::string { return (i + 1 < argc) ? argv[++i] : std::string(); };
        if (arg("--shot")) opts.shots.push_back(parseShot(next()));
        else if (arg("--frames-per-shot")) opts.framesPerShot = std::atoi(next().c_str());
        else if (arg("--width")) opts.width = std::atoi(next().c_str());
        else if (arg("--height")) opts.height = std::atoi(next().c_str());
        else if (arg("--help") || arg("-h")) { printHelp(argv[0]); return 0; }
        else { std::fprintf(stderr, "unknown option: %s\n", argv[i]); printHelp(argv[0]); return 1; }
    }

    try {
        App app(opts);
        app.run();
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "fatal: %s\n", ex.what());
        return 1;
    }
    return 0;
}
