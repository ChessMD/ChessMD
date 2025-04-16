#ifndef PGNGAMEDATA_H
#define PGNGAMEDATA_H


#include <string>
#include <vector>
#include <memory>
#include <utility>

class VariationNode
{
public:
    std::vector<std::string> moves;
    std::vector<std::pair<int,std::unique_ptr<VariationNode>>> variations;
    int plyCount;
};


class PGNGameData
{
public:
    PGNGameData(){
        rootVariation = std::make_unique<VariationNode>();
    }
    void addHeader(const std::string &tag, const std::string &value);
    void printHeader();
    void printGameTree();
    VariationNode* getRootVariation() { return rootVariation.get(); }
    std::vector<std::pair<std::string,std::string>> headerInfo;
private:
    std::unique_ptr<VariationNode> rootVariation;
};

#endif // PGNGAMEDATA_H
