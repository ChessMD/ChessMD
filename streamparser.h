/* 
March 3, 2025: File Creation
*/

#include <vector>
#include <iostream>
#include <istream>

#include "pgngamedata.h"

class StreamParser
{

public:
    StreamParser(std::istream &stream) : streamBuffer(stream) {}
    std::vector<PGNGameData> parseDatabase(void);
private:
    std::istream &streamBuffer;
};
