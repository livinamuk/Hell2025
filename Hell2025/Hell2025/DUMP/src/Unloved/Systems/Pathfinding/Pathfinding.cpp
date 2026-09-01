/*#include "Pathfinding.h"
#include "Hell/Backend/BackEnd.h"
#include "Hell/Audio.h"
#include "../Core/Input.h"
#include "../File/JSON.hpp"
#include "../Renderer/RendererCommon.h"
#include "../Util.hpp"
#include <algorithm>
#include "../Timer.hpp"
namespace Pathfinding {

    int g_mapWidth = 0;
    int g_mapHeight = 0;
    ivec2 g_start;
    ivec2 g_target;
    std::vector<int> g_map;

    AStar g_AStar;
    bool g_slowMode = true;

    void Init() {
        g_mapWidth = PRESENT_WIDTH / CELL_SIZE;
        g_mapHeight = PRESENT_HEIGHT / CELL_SIZE + 1;
        g_map.assign(g_mapWidth * g_mapHeight, 0);

        LoadMap();
    }

    void ResetAStar() {
        g_AStar.ClearData();
    }

    void Update(float deltaTime) {

        if (Input::LeftMouseDown()) {
            SetObstacle(GetMouseCellX(), GetMouseCellY(), true);
            ResetAStar();
        }
        if (Input::RightMouseDown()) {
            SetObstacle(GetMouseCellX(), GetMouseCellY(), false);
            ResetAStar();
        }
        if (Input::KeyPressed(HELL_KEY_L)) {
            Audio::PlayAudio("SELECT.wav", 1.0);
            LoadMap();
            ResetAStar();
        }
        if (Input::KeyPressed(HELL_KEY_S)) {
            Audio::PlayAudio("SELECT.wav", 1.0);
            SaveMap();
        }
        if (Input::KeyPressed(HELL_KEY_N)) {
            Audio::PlayAudio("SELECT.wav", 1.0);
            ClearMap();
            ResetAStar();
        }
        if (Input::KeyPressed(HELL_KEY_D)) {
            Audio::PlayAudio("SELECT.wav", 1.0);
            g_slowMode = !g_slowMode;
            ResetAStar();
        }
        if (Input::KeyPressed(HELL_KEY_1)) {
            Audio::PlayAudio("SELECT.wav", 1.0);
            SetStart(GetMouseCellX(), GetMouseCellY());
            ResetAStar();
        }
        if (Input::KeyPressed(HELL_KEY_2)) {
            Audio::PlayAudio("SELECT.wav", 1.0);
            SetTarget(GetMouseCellX(), GetMouseCellY());
            ResetAStar();
        }
        if (Input::KeyPressed(HELL_KEY_SPACE)) {
            Audio::PlayAudio("SELECT.wav", 1.0);
        }
        if (Input::KeyDown(HELL_KEY_SPACE) && !g_AStar.GridPathFound()) {
            Audio::PlayAudio("UI_Select.wav", 0.5);
            Timer timer("Find path");
            if (!g_AStar.SearchInitilized()) {
                g_AStar.InitSearch(g_start.x, g_start.y, g_target.x, g_target.y);
            }
            if (!g_AStar.GridPathFound()) {
                g_AStar.FindPath();
            }
        }
        if (Input::KeyPressed(HELL_KEY_W) || Input::KeyPressed(HELL_KEY_A)) {
            Audio::PlayAudio("SELECT.wav", 1.0);
        }
        if (Input::KeyDown(HELL_KEY_W) && !g_AStar.SmoothPathFound() || Input::KeyPressed(HELL_KEY_A)) {
            Audio::PlayAudio("UI_Select.wav", 0.5);
            Timer timer("Smooth path");
            if (!g_AStar.SmoothPathFound()) {
                g_AStar.FindSmoothPath();
            }
        }
    }

    void ClearMap() {
        for (int y = 0; y < GetMapWidth(); ++y) {
            for (int x = 0; x < GetMapHeight(); ++x) {
                int idx = Index1D(x, y);
                g_map[idx] = 0;
            }
        }
        g_start = { 0,0 };
        g_target = { 0,1 };
    }

    void LoadMap() {
        ClearMap();
        std::string fullPath = "res/maps/mappp.txt";
        if (Hell::File::Exists(fullPath)) {
            std::cout << "Loading map '" << fullPath << "'\n";
            std::ifstream file(fullPath);
            std::stringstream buffer;
            buffer << file.rdbuf();
            if (buffer) {
                nlohmann::json data = nlohmann::json::parse(buffer.str());
                for (const auto& jsonObject : data["map"]) {
                    int x = jsonObject["position"]["x"];
                    int y = jsonObject["position"]["y"];
                    SetObstacle(x, y, true);
                }
                g_start.x = data["start"]["x"];
                g_start.y = data["start"]["y"];
                g_target.x = data["target"]["x"];
                g_target.y = data["target"]["y"];
            }
        }
    }

    void SaveMap() {
        JSONObject saveFile;
        nlohmann::json data;
        nlohmann::json jsonMap = nlohmann::json::array();
        for (int x = 0; x < GetMapWidth(); x++) {
            for (int y = 0; y < GetMapHeight(); y++) {
                if (IsObstacle(x, y)) {
                    nlohmann::json jsonObject;
                    jsonObject["position"] = { {"x", x}, {"y", y} };
                    jsonMap.push_back(jsonObject);
                }
            }
        }
        data["map"] = jsonMap;
        data["start"] = { {"x", g_start.x}, {"y", g_start.y} };
        data["target"] = { {"x", g_target.x}, {"y", g_target.y} };
        int indent = 4;
        std::string text = data.dump(indent);
        std::cout << text << "\n\n";
        std::ofstream out("res/maps/mappp.txt");
        out << text;
        out.close();
        std::cout << "Saving map\n";
    }

    void SetStart(int x, int y) {
        if (IsInBounds(x, y)) {
            g_start = { x , y };
        }
    }

    void SetTarget(int x, int y) {
        if (IsInBounds(x, y)) {
            g_target = { x , y };
        }
    }

    bool IsInBounds(int x, int y) {
        return (x >= 0 && y >= 0 && x < g_mapWidth && y < g_mapHeight);
    }

    void SetObstacle(int x, int y, bool value) {
        if (IsInBounds(x, y)) {
            g_map[Index1D(x, y)] = value ? 1 : 0;
        }
    }

    bool IsObstacle(int x, int y) {
        if (IsInBounds(x, y)) {
            return g_map[Index1D(x, y)];
        }
        else {
            return false;
        }
    }

    int GetMouseX() {
        float scalingRatio = Hell::BackEnd::GetCurrentWindowWidth() / PRESENT_WIDTH;
        return Hell::Math::MapRange(Input::GetMouseX(), 0, Hell::BackEnd::GetCurrentWindowWidth(), 0, PRESENT_WIDTH);
    }

    int GetMouseY() {
        return Hell::Math::MapRange(Input::GetMouseY(), 0, Hell::BackEnd::GetCurrentWindowHeight(), 0, PRESENT_HEIGHT);
    }

    int GetMouseCellX() {
        return Hell::Math::MapRange(Input::GetMouseX(), 0, Hell::BackEnd::GetCurrentWindowWidth(), 0, PRESENT_WIDTH) / CELL_SIZE;
    }

    int GetMouseCellY() {
        return Hell::Math::MapRange(Input::GetMouseY(), 0, Hell::BackEnd::GetCurrentWindowHeight(), 0, PRESENT_HEIGHT) / CELL_SIZE;
    }

    int GetMapWidth() {
        return g_mapWidth;
    }

    int GetMapHeight() {
        return g_mapHeight;
    }

    int GetStartX() {
        return g_start.x;
    }

    int GetStartY() {
        return g_start.y;
    }

    int GetTargetX() {
        return g_target.x;
    }

    int GetTargetY() {
        return g_target.y;
    }

    AStar& GetAStar() {
        return g_AStar;
    }

    bool SlowModeEnabled() {
        return g_slowMode;
    }

    bool HasLineOfSight(glm::vec2 startPosition, glm::vec2 endPosition) {
        float stepSize = 0.5f;
        glm::vec2 direction = glm::normalize(endPosition - startPosition);
        glm::vec2 testPosition = startPosition;
        float distanceToEnd = glm::distance(testPosition, endPosition);
        float currentDistance = distanceToEnd;
        while (currentDistance > 1) {
            testPosition += direction * stepSize;
            if (Pathfinding::IsObstacle((int)testPosition.x, (int)testPosition.y)) {
                return false;
            }
            currentDistance = glm::distance(testPosition, endPosition);
            // You made it past your target! There is line of sight
            if (currentDistance > distanceToEnd) {
                return true;
            }
        }
        return true;
    }
}


*/
