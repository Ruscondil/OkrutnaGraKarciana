#include "cardImport.hpp"

#include <fstream>
#include <jsoncpp/json/json.h>

bool CardImporter::loadFromFile(const std::string &fileName)
{
    std::ifstream file(fileName);
    Json::CharReaderBuilder builder;
    std::string errs;
    bool ok = Json::parseFromStream(builder, file, &json, &errs);
    if (!ok)
    {
        return false;
    }
    calls.clear();
    responses.clear();
    for (Json::Value::ArrayIndex i = 0; i < json["calls"].size(); i++)
    {
        std::vector<std::string> text;
        for (Json::Value::ArrayIndex j = 0; j < json["calls"][i]["text"].size(); j++)
        {
            text.push_back(json["calls"][i]["text"][j].asString());
        }
        calls.push_back(text);
    }

    for (Json::Value::ArrayIndex i = 0; i < json["responses"].size(); i++)
    {
        responses.push_back(json["responses"][i]["text"][0].asString());
    }
    return true;
}

std::vector<std::vector<std::string>> CardImporter::getCalls()
{
    return calls;
}

std::vector<std::string> CardImporter::getResponses()
{
    return responses;
}

std::string CardImporter::cardIntoString(std::vector<std::string> card)
{
    std::string output;

    for (size_t i = 0; i < card.size() - 1; i++)
    {
        output += card[i];

        if (i == card.size() - 2)
        {
            if (card[i].back() != '?' and card[i].back() != '!' and card[i].back() != '.')
                output += " _ ";
            else
                break;
        }
        else
            output += " _ ";
    }

    output += card[card.size() - 1];

    return output;
}

int CardImporter::getCallGaps(std::vector<std::string> card)
{
    return card.size() - 1;
}