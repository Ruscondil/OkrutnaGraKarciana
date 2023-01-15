#pragma once

#include <vector>
#include <string>
#include <jsoncpp/json/json.h>

class CardImporter
{
    std::vector<std::vector<std::string>> calls;
    std::vector<std::string> responses;
    Json::Value json;

public:
    bool loadFromFile(const std::string &fileName);
    std::vector<std::vector<std::string>> getCalls();
    std::vector<std::string> getResponses();
    std::string cardIntoString(std::vector<std::string> card);
    int getCallGaps(std::vector<std::string> card);
};