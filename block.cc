#include <vector>

#include <boost/cstdint.hpp>

#include "block.h"
#include "globaldefs.h"

#include <vector>

namespace bwtc {

MainBlock::MainBlock(std::vector<byte>* block, std::vector<uint64>* stats,
                     uint64 filled) : 
    block_(block), stats_(stats), filled_(filled) {}

MainBlock::~MainBlock() {}

} //namespace bwtc
